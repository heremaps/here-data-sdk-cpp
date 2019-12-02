/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include "StreamLayerClientImpl.h"

#include <boost/format.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/write/model/PublishDataRequest.h>
#include <olp/dataservice/write/model/PublishSdiiRequest.h>
#include "ApiClientLookup.h"
#include "generated/BlobApi.h"
#include "generated/ConfigApi.h"
#include "generated/IngestApi.h"
#include "generated/PublishApi.h"

// clang-format off
#include <generated/serializer/PublishDataRequestSerializer.h>
#include <generated/serializer/JsonSerializer.h>
// clang-format on

// clang-format off
#include <generated/parser/PublishDataRequestParser.h>
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

#include <olp/core/client/TaskContext.h>
#include <olp/core/client/PendingRequests.h>

using namespace olp::client;
using namespace olp::dataservice::write::model;

namespace olp {
namespace dataservice {
namespace write {

namespace {
constexpr auto kLogTag = "StreamLayerClientImpl";
constexpr int64_t kTwentyMib = 20971520;  // 20 MiB

std::string GenerateUuid() {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}

using CallFuncType = thread::TaskScheduler::CallFuncType;

inline void ExecuteOrSchedule(
    const std::shared_ptr<thread::TaskScheduler>& task_scheduler,
    CallFuncType&& func) {
  if (!task_scheduler) {
    // User didn't specify a TaskScheduler, execute sync
    func();
  } else {
    task_scheduler->ScheduleTask(std::move(func));
  }
}

inline void ExecuteOrSchedule(const client::OlpClientSettings* settings,
                              CallFuncType&& func) {
  ExecuteOrSchedule(settings ? settings->task_scheduler : nullptr,
                    std::move(func));
}

template <typename API, typename Response, typename... Args>
Response AsyncApi(API api, Response unused_resp, client::HRN catalog,
                  client::CancellationContext context,
                  client::OlpClientSettings settings, std::string service,
                  std::string version, Args... args) {
  auto api_response = ApiClientLookup::LookupApiClient(
      catalog, context, service, version, settings);

  if (!api_response.IsSuccessful()) {
    return api_response.GetError();
  }

  auto client = api_response.GetResult();

  olp::client::Condition condition;
  auto flag = std::make_shared<std::atomic_bool>(true);
  Response response;

  auto callback = [&, flag](Response inner_response) {
    if (flag->exchange(false)) {
      response = std::move(inner_response);
      condition.Notify();
    }
  };

  context.ExecuteOrCancelled(
      [&, flag]() {
        auto token = api(client, args..., callback);
        return olp::client::CancellationToken([&, token, flag]() {
          if (flag->exchange(false)) {
            token.Cancel();
            condition.Notify();
          }
        });
      },
      [&]() { condition.Notify(); });

  if (!condition.Wait()) {
    context.CancelOperation();
    OLP_SDK_LOG_INFO_F(kLogTag, "timeout");
    return {{ErrorCode::RequestTimeout, "Network request timed out."}};
  }

  flag->store(false);

  if (context.IsCancelled()) {
    return {{ErrorCode::Cancelled, "Operation cancelled."}};
  }

  return response;
}

}  // namespace

StreamLayerClientImpl::StreamLayerClientImpl(
    HRN catalog, StreamLayerClientSettings client_settings,
    OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      settings_(std::move(settings)),
      cache_(settings_.cache),
      cache_mutex_(),
      stream_client_settings_(std::move(client_settings)),
      pending_requests_(std::make_shared<client::PendingRequests>()),
      task_scheduler_(std::move(settings_.task_scheduler)) {}

std::string StreamLayerClientImpl::FindContentTypeForLayerId(
    const model::Catalog& catalog, const std::string& layer_id) {
  std::string content_type;
  for (auto layer : catalog.GetLayers()) {
    if (layer.GetId() == layer_id) {
      // TODO optimization opportunity - cache
      // content-type for layer when found for O(1)
      // subsequent lookup.
      content_type = layer.GetContentType();
      break;
    }
  }
  return content_type;
}

StreamLayerClientImpl::~StreamLayerClientImpl() {
  pending_requests_->CancelAllAndWait();
}

std::string StreamLayerClientImpl::GetUuidListKey() const {
  const static std::string kStreamCachePostfix = "-stream-queue-cache";
  const std::string uuid_list_key =
      catalog_.ToCatalogHRNString() + kStreamCachePostfix;
  return uuid_list_key;
}

size_t StreamLayerClientImpl::QueueSize() const {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  const auto uuid_list_any =
      cache_->Get(GetUuidListKey(), [](const std::string& s) { return s; });
  std::string uuid_list = "";
  if (!uuid_list_any.empty()) {
    uuid_list = boost::any_cast<std::string>(uuid_list_any);
    return std::count(uuid_list.cbegin(), uuid_list.cend(), ',');
  }

  return 0;
}

CancellableFuture<PublishDataResponse> StreamLayerClientImpl::PublishData(
    const model::PublishDataRequest& request) {
  auto promise = std::make_shared<std::promise<PublishDataResponse> >();
  auto cancel_token =
      PublishData(request, [promise](PublishDataResponse response) {
        promise->set_value(std::move(response));
      });
  return CancellableFuture<PublishDataResponse>(cancel_token, promise);
}

boost::optional<std::string> StreamLayerClientImpl::Queue(
    const model::PublishDataRequest& request) {
  if (!cache_) {
    return boost::make_optional<std::string>(
        "No cache provided to StreamLayerClient");
  }

  if (!request.GetData()) {
    return boost::make_optional<std::string>(
        "PublishDataRequest does not contain any Data");
  }

  if (request.GetLayerId().empty()) {
    return boost::make_optional<std::string>(
        "PublishDataRequest does not contain a Layer ID");
  }

  if (!(StreamLayerClientImpl::QueueSize() <
        stream_client_settings_.maximum_requests)) {
    return boost::make_optional<std::string>(
        "Maximum number of requests has reached");
  }

  {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    const auto publish_data_key = GenerateUuid();
    cache_->Put(publish_data_key, request, [=]() {
      return olp::serializer::serialize<PublishDataRequest>(request);
    });

    const auto uuid_list_any =
        cache_->Get(GetUuidListKey(), [](const std::string& s) { return s; });
    std::string uuid_list = "";
    if (!uuid_list_any.empty()) {
      uuid_list = boost::any_cast<std::string>(uuid_list_any);
    }
    uuid_list += publish_data_key + ",";

    cache_->Put(GetUuidListKey(), uuid_list,
                [&uuid_list]() { return uuid_list; });
  }

  return boost::none;
}

boost::optional<model::PublishDataRequest>
StreamLayerClientImpl::PopFromQueue() {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  const auto uuid_list_any =
      cache_->Get(GetUuidListKey(), [](const std::string& s) { return s; });

  if (uuid_list_any.empty()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Unable to Restore UUID list from Cache");
    return boost::none;
  }

  auto uuid_list = boost::any_cast<std::string>(uuid_list_any);

  auto pos = uuid_list.find(",");
  if (pos == std::string::npos) {
    return boost::none;
  }

  const auto publish_data_key = uuid_list.substr(0, pos);
  auto publish_data_any =
      cache_->Get(publish_data_key, [](const std::string& s) {
        return olp::parser::parse<PublishDataRequest>(s);
      });

  cache_->Remove(publish_data_key);
  uuid_list.erase(0, pos + 1);
  cache_->Put(GetUuidListKey(), uuid_list,
              [&uuid_list]() { return uuid_list; });

  if (publish_data_any.empty()) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Unable to Restore PublishData Request from Cache");
    return boost::none;
  }

  return boost::any_cast<PublishDataRequest>(publish_data_any);
}

olp::client::CancellableFuture<StreamLayerClient::FlushResponse>
StreamLayerClientImpl::Flush(model::FlushRequest request) {
  auto promise =
      std::make_shared<std::promise<StreamLayerClient::FlushResponse> >();
  auto cancel_token = Flush(
      std::move(request), [promise](StreamLayerClient::FlushResponse response) {
        promise->set_value(std::move(response));
      });
  return CancellableFuture<StreamLayerClient::FlushResponse>(cancel_token,
                                                             promise);
}

olp::client::CancellationToken StreamLayerClientImpl::Flush(
    model::FlushRequest request, StreamLayerClient::FlushCallback callback) {
  CancellationContext cancel_context;

  auto self = shared_from_this();

  auto func = [=]() mutable {
    StreamLayerClient::FlushResponse response;

    const auto maximum_events_number = request.GetNumberOfRequestsToFlush();
    if (maximum_events_number < 0) {
      callback(std::move(response));
      return;
    }

    int counter = 0;
    while ((!maximum_events_number || counter < maximum_events_number) &&
           (self->QueueSize() > 0) && !cancel_context.IsCancelled()) {
      auto publish_request = self->PopFromQueue();
      if (publish_request == boost::none) {
        continue;
      }
      auto publish_response = PublishDataTask(catalog_, settings_,
                                              *publish_request, cancel_context);
      response.emplace_back(std::move(publish_response));

      // If cancelled queue back task
      if (cancel_context.IsCancelled()) {
        self->Queue(*publish_request);
        break;
      }

      counter++;
    }

    OLP_SDK_LOG_INFO_F(kLogTag, "Flushed %d publish requests", counter);
    callback(std::move(response));
  };

  ExecuteOrSchedule(task_scheduler_, func);
  return CancellationToken([=]() mutable { cancel_context.CancelOperation(); });
}

CancellationToken StreamLayerClientImpl::PublishData(
    const model::PublishDataRequest& request, PublishDataCallback callback) {
  using std::placeholders::_1;
  auto context = olp::client::TaskContext::Create(
      std::bind(&StreamLayerClientImpl::PublishDataTask, this, catalog_,
                settings_, request, _1),
      callback);

  auto pending_requests = pending_requests_;
  pending_requests->Insert(context);

  ExecuteOrSchedule(task_scheduler_, [=]() {
    context.Execute();
    pending_requests->Remove(context);
  });

  return context.CancelToken();
}

PublishDataResponse StreamLayerClientImpl::PublishDataTask(
    client::HRN catalog, client::OlpClientSettings settings,
    model::PublishDataRequest request,
    olp::client::CancellationContext context) {
  if (!request.GetData()) {
    return {{ErrorCode::InvalidArgument, "Request data null."}};
  }

  const int64_t data_size = request.GetData()->size() * sizeof(unsigned char);
  if (data_size <= kTwentyMib) {
    return PublishDataLessThanTwentyMibTask(catalog, settings, request,
                                            context);
  } else {
    return PublishDataGreaterThanTwentyMibTask(catalog, settings, request,
                                               context);
  }
}

UploadPartitionsResponse StreamLayerClientImpl::UploadPartition(
    client::HRN catalog, client::CancellationContext context,
    model::PublishDataRequest request, std::string publication_id,
    PublishPartitions partitions, std::string partition_id,
    client::OlpClientSettings settings) {
  return AsyncApi(&PublishApi::UploadPartitions, UploadPartitionsResponse{},
                  catalog, context, settings, "publish", "v2", partitions,
                  publication_id, request.GetLayerId(),
                  request.GetBillingTag());
}

IngestDataResponse StreamLayerClientImpl::IngestData(
    client::HRN catalog, client::CancellationContext context,
    model::PublishDataRequest request, std::string content_type,
    client::OlpClientSettings settings) {
  return AsyncApi(&IngestApi::IngestData, IngestDataResponse{}, catalog,
                  context, settings, "ingest", "v1", request.GetLayerId(),
                  content_type, request.GetData(), request.GetTraceId(),
                  request.GetBillingTag(), request.GetChecksum());
}

PublishSdiiResponse StreamLayerClientImpl::IngestSDII(
    client::HRN catalog, client::CancellationContext context,
    model::PublishSdiiRequest request, client::OlpClientSettings settings) {
  return AsyncApi(&IngestApi::IngestSDII, PublishSdiiResponse{}, catalog,
                  context, settings, "ingest", "v1", request.GetLayerId(),
                  request.GetSdiiMessageList(), request.GetTraceId(),
                  request.GetBillingTag(), request.GetChecksum());
}

PutBlobResponse StreamLayerClientImpl::PutBlob(
    client::HRN catalog, client::CancellationContext context,
    model::PublishDataRequest request, std::string content_type,
    std::string data_handle, client::OlpClientSettings settings) {
  return AsyncApi(&BlobApi::PutBlob, PutBlobResponse{}, catalog, context,
                  settings, "blob", "v1", request.GetLayerId(), content_type,
                  data_handle, request.GetData(), request.GetBillingTag());
}

PublishDataResponse StreamLayerClientImpl::PublishDataLessThanTwentyMibTask(
    client::HRN catalog, client::OlpClientSettings settings,
    model::PublishDataRequest request,
    olp::client::CancellationContext cancellation_context) {
  auto catalog_responce =
      GetCatalog(catalog, cancellation_context, request, settings);

  if (!catalog_responce.IsSuccessful()) {
    return catalog_responce.GetError();
  }
  auto content_type = FindContentTypeForLayerId(catalog_responce.GetResult(),
                                                request.GetLayerId());

  if (content_type.empty()) {
    auto message = boost::format(
                       "Unable to find the Layer ID (%1%) "
                       "provided in the PublishDataRequest in the "
                       "Catalog specified when creating this "
                       "StreamLayerClient instance.") %
                   request.GetLayerId();

    return {{ErrorCode::InvalidArgument, message.str()}};
  }

  auto ingest_data_response = IngestData(catalog, cancellation_context, request,
                                         content_type, settings);
  return ingest_data_response;
}

PublishDataResponse StreamLayerClientImpl::PublishDataGreaterThanTwentyMibTask(
    client::HRN catalog, client::OlpClientSettings settings,
    model::PublishDataRequest request,
    olp::client::CancellationContext cancellation_context) {
  auto catalog_responce =
      GetCatalog(catalog, cancellation_context, request, settings);

  if (!catalog_responce.IsSuccessful()) {
    return catalog_responce.GetError();
  }

  auto publication_response =
      InitPublication(catalog, cancellation_context, request, settings);
  if (!publication_response.IsSuccessful() ||
      !publication_response.GetResult().GetId()) {
    return publication_response.GetError();
  }

  auto publication = publication_response.GetResult();
  auto content_type = FindContentTypeForLayerId(catalog_responce.GetResult(),
                                                request.GetLayerId());

  if (content_type.empty()) {
    auto message = boost::format(
                       "Unable to find the Layer ID (%1%) "
                       "provided in the PublishDataRequest in the "
                       "Catalog specified when creating this "
                       "StreamLayerClient instance.") %
                   request.GetLayerId();

    return {{ErrorCode::InvalidArgument, message.str()}};
  }

  const auto data_handle = GenerateUuid();
  const auto publication_id = publication.GetId().get();

  auto put_blob_response = PutBlob(catalog, cancellation_context, request,
                                   content_type, data_handle, settings);

  if (!put_blob_response.IsSuccessful()) {
    return put_blob_response.GetError();
  }

  PublishPartition publish_partition;
  const auto partition_id = GenerateUuid();
  publish_partition.SetPartition(partition_id);
  publish_partition.SetDataHandle(data_handle);
  PublishPartitions partitions;
  partitions.SetPartitions({publish_partition});

  auto upload_partition_response =
      UploadPartition(catalog, cancellation_context, request, publication_id,
                      partitions, partition_id, settings);

  if (!upload_partition_response.IsSuccessful()) {
    return upload_partition_response.GetError();
  }

  auto submit_publication_response = SubmitPublication(
      catalog, cancellation_context, request, publication_id, settings);

  if (!submit_publication_response.IsSuccessful()) {
    return submit_publication_response.GetError();
  }
  model::ResponseOkSingle response_ok_single;
  response_ok_single.SetTraceID(partition_id);
  return response_ok_single;
}

std::string CreateCatalogKey(const std::string& hrn) {
  return hrn + "::catalog";
}

CatalogResponse StreamLayerClientImpl::GetCatalog(
    client::HRN catalog, client::CancellationContext context,
    model::PublishDataRequest request, client::OlpClientSettings settings) {
  return AsyncApi(&ConfigApi::GetCatalogB, CatalogResponse{}, catalog, context,
                  settings, "config", "v1", catalog.ToCatalogHRNString(),
                  request.GetBillingTag());
}

InitPublicationResponse StreamLayerClientImpl::InitPublication(
    client::HRN catalog, client::CancellationContext context,
    model::PublishDataRequest request, client::OlpClientSettings settings) {
  Publication pub;
  pub.SetLayerIds({request.GetLayerId()});

  return AsyncApi(&PublishApi::InitPublication, InitPublicationResponse{},
                  catalog, context, settings, "publish", "v2", pub,
                  request.GetBillingTag());
}

SubmitPublicationResponse StreamLayerClientImpl::SubmitPublication(
    client::HRN catalog, client::CancellationContext context,
    model::PublishDataRequest request, std::string publication_id,
    client::OlpClientSettings settings) {
  return AsyncApi(&PublishApi::SubmitPublication, SubmitPublicationResponse{},
                  catalog, context, settings, "publish", "v2", publication_id,
                  request.GetBillingTag());
}

CancellableFuture<PublishSdiiResponse> StreamLayerClientImpl::PublishSdii(
    const model::PublishSdiiRequest& request) {
  auto promise = std::make_shared<std::promise<PublishSdiiResponse> >();
  auto cancel_token =
      PublishSdii(request, [promise](PublishSdiiResponse response) {
        promise->set_value(std::move(response));
      });
  return CancellableFuture<PublishSdiiResponse>(cancel_token, promise);
}

CancellationToken StreamLayerClientImpl::PublishSdii(
    const model::PublishSdiiRequest& request, PublishSdiiCallback callback) {
  using std::placeholders::_1;
  auto context = olp::client::TaskContext::Create(
      std::bind(&StreamLayerClientImpl::PublishSdiiTask, this, catalog_,
                settings_, request, _1),
      callback);

  auto pending_requests = pending_requests_;
  pending_requests->Insert(context);

  ExecuteOrSchedule(task_scheduler_, [=]() {
    context.Execute();
    pending_requests->Remove(context);
  });

  return context.CancelToken();
}

PublishSdiiResponse StreamLayerClientImpl::PublishSdiiTask(
    client::HRN catalog, client::OlpClientSettings settings,
    model::PublishSdiiRequest request,
    olp::client::CancellationContext context) {
  if (!request.GetSdiiMessageList()) {
    return PublishSdiiResponse(ApiError(ErrorCode::InvalidArgument,
                                        "Request sdii message list null."));
  }

  if (request.GetLayerId().empty()) {
    return PublishSdiiResponse(
        ApiError(ErrorCode::InvalidArgument, "Request layer id empty."));
  }

  return AsyncApi(&IngestApi::IngestSDII, PublishSdiiResponse{}, catalog,
                  context, settings, "ingest", "v1", request.GetLayerId(),
                  request.GetSdiiMessageList(), request.GetTraceId(),
                  request.GetBillingTag(), request.GetChecksum());
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
