/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/client/TaskContext.h>
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
#include <generated/serializer/CatalogSerializer.h>
#include <generated/serializer/PublishDataRequestSerializer.h>
#include <generated/serializer/JsonSerializer.h>
// clang-format on

// clang-format off
#include <generated/parser/CatalogParser.h>
#include <generated/parser/PublishDataRequestParser.h>
#include "JsonResultParser.h"
// clang-format on

namespace olp {
namespace dataservice {
namespace write {

namespace {
constexpr auto kLogTag = "StreamLayerClientImpl";
constexpr int64_t kTwentyMib = 20971520;  // 20 MiB

void ExecuteOrSchedule(const std::shared_ptr<thread::TaskScheduler>& scheduler,
                       thread::TaskScheduler::CallFuncType&& func) {
  if (!scheduler) {
    // User didn't specify a TaskScheduler, execute sync
    func();
    return;
  }

  // Schedule for async execution
  scheduler->ScheduleTask(std::move(func));
}
}  // namespace

StreamLayerClientImpl::StreamLayerClientImpl(
    client::HRN catalog, StreamLayerClientSettings client_settings,
    client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      settings_(std::move(settings)),
      cache_(settings_.cache),
      cache_mutex_(),
      stream_client_settings_(std::move(client_settings)),
      pending_requests_(std::make_shared<client::PendingRequests>()),
      task_scheduler_(std::move(settings_.task_scheduler)) {}

StreamLayerClientImpl::~StreamLayerClientImpl() {
  pending_requests_->CancelAllAndWait();
}

bool StreamLayerClientImpl::CancelPendingRequests() {
  OLP_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  return pending_requests_->CancelAll();
}

StreamLayerClientImpl::LayerSettingsResult
StreamLayerClientImpl::GetLayerSettings(client::CancellationContext context,
                                        BillingTag billing_tag,
                                        const std::string& layer_id) {
  const auto catalog_settings_key = catalog_.ToString() + "::catalog";

  if (!cache_->Contains(catalog_settings_key)) {
    auto lookup_response = ApiClientLookup::LookupApiClient(
        catalog_, context, "config", "v1", settings_);
    if (!lookup_response.IsSuccessful()) {
      return lookup_response.GetError();
    }

    client::OlpClient client = lookup_response.GetResult();
    auto catalog_response = ConfigApi::GetCatalog(client, catalog_.ToString(),
                                                  billing_tag, context);

    if (!catalog_response.IsSuccessful()) {
      return catalog_response.GetError();
    }

    auto catalog_model = catalog_response.MoveResult();

    cache_->Put(
        catalog_settings_key, catalog_model,
        [&]() { return serializer::serialize<model::Catalog>(catalog_model); },
        settings_.default_cache_expiration.count());
  }

  LayerSettings settings;

  const auto& cached_catalog =
      cache_->Get(catalog_settings_key, [](const std::string& model) {
        return parser::parse<model::Catalog>(model);
      });

  if (cached_catalog.empty()) {
    return settings;
  }

  const auto& catalog = boost::any_cast<const model::Catalog&>(cached_catalog);

  const auto& layers = catalog.GetLayers();

  auto layer_it = std::find_if(
      layers.begin(), layers.end(),
      [&](const model::Layer& layer) { return layer.GetId() == layer_id; });

  if (layer_it != layers.end()) {
    settings.content_type = layer_it->GetContentType();
    settings.content_encoding = layer_it->GetContentEncoding();
  }

  return settings;
}

std::string StreamLayerClientImpl::GetUuidListKey() const {
  static const std::string kStreamCachePostfix = "-stream-queue-cache";
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
      return olp::serializer::serialize<model::PublishDataRequest>(request);
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
        return olp::parser::parse<model::PublishDataRequest>(s);
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

  return boost::any_cast<model::PublishDataRequest>(publish_data_any);
}

olp::client::CancellableFuture<StreamLayerClient::FlushResponse>
StreamLayerClientImpl::Flush(model::FlushRequest request) {
  auto promise =
      std::make_shared<std::promise<StreamLayerClient::FlushResponse>>();
  auto cancel_token = Flush(
      std::move(request), [promise](StreamLayerClient::FlushResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<StreamLayerClient::FlushResponse>(
      cancel_token, promise);
}

olp::client::CancellationToken StreamLayerClientImpl::Flush(
    model::FlushRequest request, StreamLayerClient::FlushCallback callback) {
  // Because TaskContext accepts the execution callbacks which return only
  // ApiResponse object, we need to simulate the 'empty' response in order to
  // be able to use Flush execution lambda in TaskContext.
  struct EmptyFlushResponse {};
  using EmptyFlushApiResponse =
      client::ApiResponse<EmptyFlushResponse, client::ApiError>;

  // this flag is required in order to protect the user from 2 `callback`
  // invocation: one during execution phase and other when `Flush` is cancelled.
  auto exec_started = std::make_shared<std::atomic_bool>(false);

  auto task_context = client::TaskContext::Create(
      [=](client::CancellationContext context) -> EmptyFlushApiResponse {
        exec_started->exchange(true);

        StreamLayerClient::FlushResponse responses;
        const auto maximum_events_number = request.GetNumberOfRequestsToFlush();
        if (maximum_events_number < 0) {
          callback(std::move(responses));
          return EmptyFlushApiResponse{};
        }

        int counter = 0;
        while ((!maximum_events_number || counter < maximum_events_number) &&
               (this->QueueSize() > 0) && !context.IsCancelled()) {
          auto publish_request = this->PopFromQueue();
          if (publish_request == boost::none) {
            continue;
          }

          auto publish_response = PublishDataTask(*publish_request, context);
          responses.emplace_back(std::move(publish_response));

          // If cancelled queue back task
          if (context.IsCancelled()) {
            this->Queue(*publish_request);
            break;
          }

          counter++;
        }

        OLP_SDK_LOG_INFO_F(kLogTag, "Flushed %d publish requests", counter);
        callback(responses);
        return EmptyFlushApiResponse{};
      },
      [=](EmptyFlushApiResponse /*response*/) {
        // we don't need to notify user 2 times, cause we already invoke a
        // callback in the execution function:
        if (!exec_started->load()) {
          callback(StreamLayerClient::FlushResponse{});
        }
      });

  auto pending_requests = pending_requests_;
  pending_requests->Insert(task_context);

  ExecuteOrSchedule(task_scheduler_, [=]() {
    task_context.Execute();
    pending_requests->Remove(task_context);
  });

  return task_context.CancelToken();
}

olp::client::CancellableFuture<PublishDataResponse>
StreamLayerClientImpl::PublishData(model::PublishDataRequest request) {
  auto promise = std::make_shared<std::promise<PublishDataResponse>>();
  auto cancel_token =
      PublishData(std::move(request), [promise](PublishDataResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<PublishDataResponse>(cancel_token, promise);
}

client::CancellationToken StreamLayerClientImpl::PublishData(
    model::PublishDataRequest request, PublishDataCallback callback) {
  if (!request.GetData()) {
    callback(PublishDataResponse(client::ApiError(
        client::ErrorCode::InvalidArgument, "Request's data is null.")));
    return client::CancellationToken();
  }

  using std::placeholders::_1;
  client::TaskContext task_context = olp::client::TaskContext::Create(
      std::bind(&StreamLayerClientImpl::PublishDataTask, this, request, _1),
      callback);

  auto pending_requests = pending_requests_;
  pending_requests->Insert(task_context);

  ExecuteOrSchedule(task_scheduler_, [=]() {
    task_context.Execute();
    pending_requests->Remove(task_context);
  });

  return task_context.CancelToken();
}

PublishDataResponse StreamLayerClientImpl::PublishDataTask(
    model::PublishDataRequest request, client::CancellationContext context) {
  const int64_t data_size = request.GetData()->size() * sizeof(unsigned char);
  if (data_size <= kTwentyMib) {
    return PublishDataLessThanTwentyMib(std::move(request), std::move(context));
  } else {
    return PublishDataGreaterThanTwentyMib(std::move(request),
                                           std::move(context));
  }
}

PublishDataResponse StreamLayerClientImpl::PublishDataLessThanTwentyMib(
    model::PublishDataRequest request, client::CancellationContext context) {
  OLP_SDK_LOG_TRACE_F(kLogTag,
                      "Started publishing data less than 20 MB, size=%zu B",
                      request.GetData()->size());

  auto layer_settings_result =
      GetLayerSettings(context, request.GetBillingTag(), request.GetLayerId());

  if (!layer_settings_result.IsSuccessful()) {
    return layer_settings_result.GetError();
  }

  const auto& layer_settings = layer_settings_result.GetResult();

  if (layer_settings.content_type.empty()) {
    return PublishDataResponse(client::ApiError(
        client::ErrorCode::InvalidArgument,
        "Unable to find the Layer ID=`" + request.GetLayerId() +
            "` provided in the PublishDataRequest in the Catalog=" +
            catalog_.ToString()));
  }

  auto ingest_api = ApiClientLookup::LookupApiClient(catalog_, context,
                                                     "ingest", "v1", settings_);
  if (!ingest_api.IsSuccessful()) {
    return PublishDataResponse(ingest_api.GetError());
  }

  auto ingest_api_client = ingest_api.GetResult();

  auto ingest_data_response = IngestApi::IngestData(
      ingest_api_client, request.GetLayerId(), layer_settings.content_type,
      layer_settings.content_encoding, request.GetData(), request.GetTraceId(),
      request.GetBillingTag(), request.GetChecksum(), context);

  if (!ingest_data_response.IsSuccessful()) {
    return ingest_data_response;
  }

  OLP_SDK_LOG_TRACE_F(
      kLogTag,
      "Successfully published data less than 20 MB, size=%zu B, trace_id=%s",
      request.GetData()->size(),
      ingest_data_response.GetResult().GetTraceID().c_str());

  return ingest_data_response;
}

PublishDataResponse StreamLayerClientImpl::PublishDataGreaterThanTwentyMib(
    model::PublishDataRequest request, client::CancellationContext context) {
  OLP_SDK_LOG_TRACE_F(kLogTag,
                      "Started publishing data greater than 20MB, size=%zu B",
                      request.GetData()->size());

  auto layer_settings_result =
      GetLayerSettings(context, request.GetBillingTag(), request.GetLayerId());

  if (!layer_settings_result.IsSuccessful()) {
    return layer_settings_result.GetError();
  }

  const auto& layer_settings = layer_settings_result.GetResult();

  if (layer_settings.content_type.empty()) {
    return PublishDataResponse(client::ApiError(
        client::ErrorCode::InvalidArgument,
        "Unable to find the Layer ID=`" + request.GetLayerId() +
            "` provided in the PublishDataRequest in the Catalog=" +
            catalog_.ToString()));
  }

  // Init api clients for publications:
  auto publish_client_response = ApiClientLookup::LookupApiClient(
      catalog_, context, "publish", "v2", settings_);
  if (!publish_client_response.IsSuccessful()) {
    return PublishDataResponse(publish_client_response.GetError());
  }
  client::OlpClient publish_client = publish_client_response.MoveResult();

  auto blob_client_response = ApiClientLookup::LookupApiClient(
      catalog_, context, "blob", "v1", settings_);
  if (!blob_client_response.IsSuccessful()) {
    return PublishDataResponse(blob_client_response.GetError());
  }
  client::OlpClient blob_client = blob_client_response.MoveResult();

  // 1. init publication:
  model::Publication publication;
  publication.SetLayerIds({request.GetLayerId()});
  auto init_publicaion_response = PublishApi::InitPublication(
      publish_client, publication, request.GetBillingTag(), context);
  if (!init_publicaion_response.IsSuccessful()) {
    return PublishDataResponse(init_publicaion_response.GetError());
  }

  if (!init_publicaion_response.GetResult().GetId()) {
    return PublishDataResponse(
        client::ApiError(client::ErrorCode::InvalidArgument,
                         "Response from server on InitPublication request "
                         "doesn't contain any publication"));
  }
  const std::string publication_id =
      init_publicaion_response.GetResult().GetId().get();

  // 2. Put blob API:
  const auto data_handle = GenerateUuid();
  auto put_blob_response = BlobApi::PutBlob(
      blob_client, request.GetLayerId(), layer_settings.content_type,
      data_handle, request.GetData(), request.GetBillingTag(), context);
  if (!put_blob_response.IsSuccessful()) {
    return PublishDataResponse(put_blob_response.GetError());
  }

  // 3. Upload partition:
  const auto partition_id = GenerateUuid();
  model::PublishPartition publish_partition;
  publish_partition.SetPartition(partition_id);
  publish_partition.SetDataHandle(data_handle);
  model::PublishPartitions partitions;
  partitions.SetPartitions({publish_partition});

  auto upload_partitions_response = PublishApi::UploadPartitions(
      publish_client, partitions, publication_id, request.GetLayerId(),
      request.GetBillingTag(), context);
  if (!upload_partitions_response.IsSuccessful()) {
    return PublishDataResponse(upload_partitions_response.GetError());
  }

  // 4. Sumbit publication:
  auto submit_publication_response = PublishApi::SubmitPublication(
      publish_client, publication_id, request.GetBillingTag(), context);
  if (!submit_publication_response.IsSuccessful()) {
    return PublishDataResponse(submit_publication_response.GetError());
  }

  // 5. final result on successful submit of publication:
  model::ResponseOkSingle response_ok_single;
  response_ok_single.SetTraceID(partition_id);

  OLP_SDK_LOG_TRACE_F(
      kLogTag,
      "Successfully published data greater than 20 MB, size=%zu B, trace_id=%s",
      request.GetData()->size(), partition_id.c_str());
  return PublishDataResponse(response_ok_single);
}

client::CancellableFuture<PublishSdiiResponse>
StreamLayerClientImpl::PublishSdii(model::PublishSdiiRequest request) {
  auto promise = std::make_shared<std::promise<PublishSdiiResponse>>();
  auto cancel_token =
      PublishSdii(std::move(request), [promise](PublishSdiiResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<PublishSdiiResponse>(cancel_token, promise);
}

client::CancellationToken StreamLayerClientImpl::PublishSdii(
    model::PublishSdiiRequest request, PublishSdiiCallback callback) {
  using std::placeholders::_1;
  auto context = olp::client::TaskContext::Create(
      std::bind(&StreamLayerClientImpl::PublishSdiiTask, this,
                std::move(request), _1),
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
    model::PublishSdiiRequest request,
    olp::client::CancellationContext context) {
  if (!request.GetSdiiMessageList()) {
    return {{client::ErrorCode::InvalidArgument,
             "Request sdii message list null."}};
  }

  if (request.GetLayerId().empty()) {
    return {{client::ErrorCode::InvalidArgument, "Request layer id empty."}};
  }

  return IngestSdii(std::move(request), context);
}

PublishSdiiResponse StreamLayerClientImpl::IngestSdii(
    model::PublishSdiiRequest request, client::CancellationContext context) {
  auto api_response = ApiClientLookup::LookupApiClient(
      catalog_, context, "ingest", "v1", settings_);

  if (!api_response.IsSuccessful()) {
    return api_response.GetError();
  }

  auto client = api_response.MoveResult();
  return IngestApi::IngestSdii(client, request.GetLayerId(),
                               request.GetSdiiMessageList(),
                               request.GetTraceId(), request.GetBillingTag(),
                               request.GetChecksum(), context);
}

std::string StreamLayerClientImpl::GenerateUuid() const {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
