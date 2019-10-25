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

void ExecuteOrSchedule(const OlpClientSettings& settings,
                       thread::TaskScheduler::CallFuncType&& func) {
  if (!settings.task_scheduler) {
    // User didn't specify a TaskScheduler, execute sync
    func();
    return;
  }

  // Schedule for async execution
  settings.task_scheduler->ScheduleTask(std::move(func));
}
}  // namespace

StreamLayerClientImpl::StreamLayerClientImpl(
    HRN catalog, StreamLayerClientSettings client_settings,
    OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      catalog_model_(),
      settings_(std::move(settings)),
      apiclient_config_(nullptr),
      apiclient_ingest_(nullptr),
      apiclient_blob_(nullptr),
      apiclient_publish_(nullptr),
      init_mutex_(),
      init_cv_(),
      init_inprogress_(false),
      cache_(settings_.cache),
      cache_mutex_(),
      stream_client_settings_(std::move(client_settings)),
      auto_flush_controller_(new AutoFlushController(AutoFlushSettings{})) {}

CancellationToken StreamLayerClientImpl::InitApiClients(
    const std::shared_ptr<CancellationContext>& cancel_context,
    InitApiClientsCallback callback) {
  if (apiclient_config_ && !apiclient_config_->GetBaseUrl().empty()) {
    callback(boost::none);
    return CancellationToken();
  }

  apiclient_ingest_ = OlpClientFactory::Create(settings_);

  auto self = shared_from_this();
  return ApiClientLookup::LookupApi(
      apiclient_ingest_, "ingest", "v1", catalog_,
      [=](ApiClientLookup::ApisResponse apis) {
        if (!apis.IsSuccessful()) {
          callback(std::move(apis.GetError()));
          return;
        }
        self->apiclient_ingest_->SetBaseUrl(
            apis.GetResult().at(0).GetBaseUrl());

        cancel_context->ExecuteOrCancelled(
            [=]() -> CancellationToken {
              self->apiclient_config_ = OlpClientFactory::Create(settings_);

              return ApiClientLookup::LookupApi(
                  apiclient_config_, "config", "v1", catalog_,
                  [=](ApiClientLookup::ApisResponse apis) {
                    if (!apis.IsSuccessful()) {
                      callback(std::move(apis.GetError()));
                      return;
                    }
                    self->apiclient_config_->SetBaseUrl(
                        apis.GetResult().at(0).GetBaseUrl());

                    callback(boost::none);
                  });
            },
            [=]() {
              callback(
                  ApiError(ErrorCode::Cancelled, "Operation cancelled.", true));
            });
      });
}

CancellationToken StreamLayerClientImpl::InitApiClientsGreaterThanTwentyMib(
    const std::shared_ptr<CancellationContext>& cancel_context,
    InitApiClientsCallback callback) {
  if (apiclient_blob_ && !apiclient_blob_->GetBaseUrl().empty()) {
    callback(boost::none);
    return CancellationToken();
  }

  apiclient_publish_ = OlpClientFactory::Create(settings_);

  auto self = shared_from_this();
  return ApiClientLookup::LookupApi(
      apiclient_publish_, "publish", "v2", catalog_,
      [=](ApiClientLookup::ApisResponse apis) {
        if (!apis.IsSuccessful()) {
          callback(std::move(apis.GetError()));
          return;
        }
        self->apiclient_publish_->SetBaseUrl(
            apis.GetResult().at(0).GetBaseUrl());

        cancel_context->ExecuteOrCancelled(
            [=]() -> CancellationToken {
              self->apiclient_blob_ = OlpClientFactory::Create(settings_);

              return ApiClientLookup::LookupApi(
                  apiclient_blob_, "blob", "v1", catalog_,
                  [=](ApiClientLookup::ApisResponse apis) {
                    if (!apis.IsSuccessful()) {
                      callback(std::move(apis.GetError()));
                      return;
                    }
                    self->apiclient_blob_->SetBaseUrl(
                        apis.GetResult().at(0).GetBaseUrl());

                    callback(boost::none);
                  });
            },
            [=]() {
              callback(
                  ApiError(ErrorCode::Cancelled, "Operation cancelled.", true));
            });
      });
}

CancellationToken StreamLayerClientImpl::InitCatalogModel(
    const model::PublishDataRequest& request,
    const InitCatalogModelCallback& callback) {
  if (!catalog_model_.GetId().empty()) {
    callback(boost::none);
    return CancellationToken();
  }

  auto self = shared_from_this();
  return ConfigApi::GetCatalog(
      apiclient_config_, catalog_.ToString(), request.GetBillingTag(),
      [=](CatalogResponse catalog_response) {
        if (!catalog_response.IsSuccessful()) {
          callback(std::move(catalog_response.GetError()));
          return;
        }

        self->catalog_model_ = catalog_response.GetResult();

        callback(boost::none);
      });
}

void StreamLayerClientImpl::AquireInitLock() {
  std::unique_lock<std::mutex> lock(init_mutex_);
  init_cv_.wait(lock, [=] { return !init_inprogress_; });
  init_inprogress_ = true;
}

void StreamLayerClientImpl::NotifyInitAborted() {
  std::unique_lock<std::mutex> lock(init_mutex_);
  init_inprogress_ = false;
  init_cv_.notify_one();
}

void StreamLayerClientImpl::NotifyInitCompleted() {
  std::unique_lock<std::mutex> lock(init_mutex_);
  init_inprogress_ = false;
  init_cv_.notify_all();
}

std::string StreamLayerClientImpl::FindContentTypeForLayerId(
    const std::string& layer_id) {
  std::string content_type;
  for (auto layer : catalog_model_.GetLayers()) {
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

  if (stream_client_settings_.maximum_requests) {
    if (!(StreamLayerClientImpl::QueueSize() <
          *stream_client_settings_.maximum_requests)) {
      return boost::make_optional<std::string>(
          "Maximum number of requests has reached");
    }
  }

  auto_flush_controller_->NotifyQueueEventStart();
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
  auto_flush_controller_->NotifyQueueEventComplete();

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

      self->auto_flush_controller_->NotifyFlushEvent();

      // TODO: This needs a redesign as pushing multiple publishes also on the
      // TaskScheduler would mean a dead-lock in case we have a single-thread
      // pool and this is waiting on each publish to finish. So while this loop
      // here is active we will not be able to redesign PublishData() internally
      // to use the TaskScheduler.
      std::promise<void> barrier;
      cancel_context.ExecuteOrCancelled(
          [&]() -> CancellationToken {
            return self->PublishData(
                *publish_request, [&](PublishDataResponse publish_response) {
                  response.emplace_back(std::move(publish_response));
                  barrier.set_value();
                });
          },
          [&barrier]() { barrier.set_value(); });

      barrier.get_future().get();

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

  ExecuteOrSchedule(settings_, func);
  return CancellationToken([=]() mutable { cancel_context.CancelOperation(); });
}

CancellationToken StreamLayerClientImpl::PublishData(
    const model::PublishDataRequest& request, PublishDataCallback callback) {
  if (!request.GetData()) {
    callback(PublishDataResponse(
        ApiError(ErrorCode::InvalidArgument, "Request data null.")));
    return CancellationToken();
  }

  const int64_t data_size = request.GetData()->size() * sizeof(unsigned char);
  if (data_size <= kTwentyMib) {
    return PublishDataLessThanTwentyMib(request, callback);
  } else {
    return PublishDataGreaterThanTwentyMib(request, callback);
  }
}

CancellationToken StreamLayerClientImpl::PublishDataLessThanTwentyMib(
    const model::PublishDataRequest& request,
    const PublishDataCallback& callback) {
  // Publish < 20MB init is complete when catalog model is valid
  if (catalog_model_.GetId().empty()) {
    AquireInitLock();
  }

  auto cancel_context = std::make_shared<CancellationContext>();
  auto self = shared_from_this();
  cancel_context->ExecuteOrCancelled(
      [=]() -> CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<ApiError> init_api_error) {
              if (init_api_error) {
                NotifyInitAborted();
                callback(PublishDataResponse(init_api_error.get()));
                return;
              }

              cancel_context->ExecuteOrCancelled(
                  [=]() -> CancellationToken {
                    return self->InitCatalogModel(
                        request, [=](boost::optional<ApiError>
                                         init_catalog_model_error) {
                          if (init_catalog_model_error) {
                            NotifyInitAborted();
                            callback(PublishDataResponse(
                                init_catalog_model_error.get()));
                            return;
                          }

                          NotifyInitCompleted();

                          auto content_type = self->FindContentTypeForLayerId(
                              request.GetLayerId());
                          if (content_type.empty()) {
                            callback(PublishDataResponse(ApiError(
                                ErrorCode::InvalidArgument,
                                "Unable to find the Layer ID provided in "
                                "the PublishDataRequest in the "
                                "Catalog specified when creating this "
                                "StreamLayerClient instance.")));
                            return;
                          }

                          cancel_context->ExecuteOrCancelled(
                              [=]() -> CancellationToken {
                                return IngestApi::IngestData(
                                    *self->apiclient_ingest_,
                                    request.GetLayerId(), content_type,
                                    request.GetData(), request.GetTraceId(),
                                    request.GetBillingTag(),
                                    request.GetChecksum(),
                                    [callback](IngestDataResponse response) {
                                      callback(std::move(response));
                                    });
                              },
                              [=]() {
                                callback(PublishDataResponse(
                                    ApiError(ErrorCode::Cancelled,
                                             "Operation cancelled.", true)));
                                return;
                              });
                        });
                  },
                  [=]() {
                    callback(PublishDataResponse(ApiError(
                        ErrorCode::Cancelled, "Operation cancelled.", true)));
                  });
            });
      },
      [=]() {
        callback(PublishDataResponse(
            ApiError(ErrorCode::Cancelled, "Operation cancelled.", true)));
      });

  std::weak_ptr<StreamLayerClientImpl> weak_self{self};
  return CancellationToken([cancel_context, weak_self]() {
    cancel_context->CancelOperation();
    if (auto strong_self = weak_self.lock()) {
      strong_self->NotifyInitAborted();
    }
  });
}

void StreamLayerClientImpl::InitPublishDataGreaterThanTwentyMib(
    const std::shared_ptr<client::CancellationContext>& cancel_context,
    const model::PublishDataRequest& request,
    const PublishDataCallback& callback) {
  // Publish > 20MB init is complete when catalog model AND apiclient_blob_ are
  // valid
  if (catalog_model_.GetId().empty() ||
      (!apiclient_blob_ || apiclient_blob_->GetBaseUrl().empty())) {
    AquireInitLock();
  }

  auto self = shared_from_this();
  cancel_context->ExecuteOrCancelled(
      [=]() -> CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<ApiError> init_api_error) {
              if (init_api_error) {
                NotifyInitAborted();
                callback(PublishDataResponse(init_api_error.get()));
                return;
              }

              cancel_context->ExecuteOrCancelled(
                  [=]() -> CancellationToken {
                    return self->InitApiClientsGreaterThanTwentyMib(
                        cancel_context,
                        [=](boost::optional<ApiError> init_api_error) {
                          if (init_api_error) {
                            NotifyInitAborted();
                            callback(PublishDataResponse(init_api_error.get()));
                            return;
                          }

                          cancel_context->ExecuteOrCancelled(
                              [=]() -> CancellationToken {
                                return self->InitCatalogModel(
                                    request, [=](boost::optional<ApiError>
                                                     init_catalog_model_error) {
                                      if (init_catalog_model_error) {
                                        NotifyInitAborted();
                                        callback(PublishDataResponse(
                                            init_catalog_model_error.get()));
                                        return;
                                      }

                                      NotifyInitCompleted();

                                      // Empty success response indicates
                                      // initialization completed successfully.
                                      callback(PublishDataResponse(
                                          ResponseOkSingle()));
                                    });
                              },
                              [=]() {
                                callback(PublishDataResponse(
                                    ApiError(ErrorCode::Cancelled,
                                             "Operation cancelled.", true)));
                              });
                        });
                  },
                  [=]() {
                    callback(PublishDataResponse(ApiError(
                        ErrorCode::Cancelled, "Operation cancelled.", true)));
                  });
            });
      },
      [=]() {
        callback(PublishDataResponse(
            ApiError(ErrorCode::Cancelled, "Operation cancelled.", true)));
      });
}

CancellationToken StreamLayerClientImpl::PublishDataGreaterThanTwentyMib(
    const model::PublishDataRequest& request,
    const PublishDataCallback& callback) {
  auto self = shared_from_this();
  auto cancel_context = std::make_shared<CancellationContext>();
  auto cancel_function = [callback]() {
    callback(PublishDataResponse(
        ApiError(ErrorCode::Cancelled, "Operation cancelled.", true)));
  };

  // Publishing data greater than 20MiBs requires several steps: 1.
  // Initialze the Publication, 2. Upload the Data 3. Upload publication
  // metadata 4. Submit the publication. The functions to perform these steps
  // are decomposed into anonoymous functions to avoid callback hell. They are
  // declared in order from last called to first called.

  auto submit_publication_callback =
      [=](SubmitPublicationResponse submit_publication_response,
          std::string partition_id) {
        if (!submit_publication_response.IsSuccessful()) {
          callback(PublishDataResponse(submit_publication_response.GetError()));
          return;
        }
        model::ResponseOkSingle response_ok_single;
        response_ok_single.SetTraceID(partition_id);
        callback(PublishDataResponse(response_ok_single));
      };

  auto submit_publication_function =
      [=](std::string publication_id,
          std::string partition_id) -> CancellationToken {
    return PublishApi::SubmitPublication(
        *self->apiclient_publish_, publication_id, request.GetBillingTag(),
        std::bind(submit_publication_callback, std::placeholders::_1,
                  partition_id));
  };

  auto upload_partitions_callback =
      [=](UploadPartitionsResponse upload_partitions_response,
          std::string publication_id, std::string partition_id) {
        if (!upload_partitions_response.IsSuccessful()) {
          callback(PublishDataResponse(upload_partitions_response.GetError()));
          return;
        }

        cancel_context->ExecuteOrCancelled(
            std::bind(submit_publication_function, publication_id,
                      partition_id),
            cancel_function);
      };

  auto upload_partitions_function =
      [=](std::string publication_id, PublishPartitions partitions,
          std::string partition_id) -> CancellationToken {
    return PublishApi::UploadPartitions(
        *self->apiclient_publish_, partitions, publication_id,
        request.GetLayerId(), request.GetBillingTag(),
        std::bind(upload_partitions_callback, std::placeholders::_1,
                  publication_id, partition_id));
  };

  auto put_blob_callback = [=](PutBlobResponse put_blob_response,
                               std::string publication_id,
                               std::string data_handle) {
    if (!put_blob_response.IsSuccessful()) {
      callback(PublishDataResponse(put_blob_response.GetError()));
      return;
    }

    const auto partition_id = GenerateUuid();
    PublishPartition publish_partition;
    publish_partition.SetPartition(partition_id);
    publish_partition.SetDataHandle(data_handle);
    PublishPartitions partitions;
    partitions.SetPartitions({publish_partition});

    cancel_context->ExecuteOrCancelled(
        std::bind(upload_partitions_function, publication_id, partitions,
                  partition_id),
        cancel_function);
  };

  auto put_blob_function = [=](std::string publication_id,
                               std::string content_type,
                               std::string data_handle) -> CancellationToken {
    return BlobApi::PutBlob(*self->apiclient_blob_, request.GetLayerId(),
                            content_type, data_handle, request.GetData(),
                            request.GetBillingTag(),
                            std::bind(put_blob_callback, std::placeholders::_1,
                                      publication_id, data_handle));
  };

  auto init_publication_callback =
      [=](InitPublicationResponse init_pub_response) {
        if (!init_pub_response.IsSuccessful() ||
            !init_pub_response.GetResult().GetId()) {
          callback(PublishDataResponse(init_pub_response.GetError()));
          return;
        }

        auto content_type =
            self->FindContentTypeForLayerId(request.GetLayerId());
        if (content_type.empty()) {
          auto errmsg = boost::format(
                            "Unable to find the Layer ID (%1%) "
                            "provided in the PublishDataRequest in the "
                            "Catalog specified when creating this "
                            "StreamLayerClient instance.") %
                        request.GetLayerId();
          callback(PublishDataResponse(
              ApiError(ErrorCode::InvalidArgument, errmsg.str())));
          return;
        }

        const auto data_handle = GenerateUuid();

        cancel_context->ExecuteOrCancelled(
            std::bind(put_blob_function,
                      init_pub_response.GetResult().GetId().get(), content_type,
                      data_handle),
            cancel_function);
      };

  auto init_publication_function = [=]() -> CancellationToken {
    Publication pub;
    pub.SetLayerIds({request.GetLayerId()});

    return PublishApi::InitPublication(*self->apiclient_publish_, pub,
                                       request.GetBillingTag(),
                                       init_publication_callback);
  };

  auto post_intialize_callback = [=](PublishDataResponse response) {
    if (response.IsSuccessful()) {
      cancel_context->ExecuteOrCancelled(init_publication_function,
                                         cancel_function);
    } else {
      callback(response);
    }
  };

  InitPublishDataGreaterThanTwentyMib(cancel_context, request,
                                      post_intialize_callback);

  std::weak_ptr<StreamLayerClientImpl> weak_self{self};
  return CancellationToken([cancel_context, weak_self]() {
    cancel_context->CancelOperation();
    if (auto strong_self = weak_self.lock()) {
      strong_self->NotifyInitAborted();
    }
  });
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
  if (!request.GetSdiiMessageList()) {
    callback(PublishSdiiResponse(ApiError(ErrorCode::InvalidArgument,
                                          "Request sdii message list null.")));
    return CancellationToken();
  }

  if (request.GetLayerId().empty()) {
    callback(PublishSdiiResponse(
        ApiError(ErrorCode::InvalidArgument, "Request layer id empty.")));
    return CancellationToken();
  }

  // Sdii publish init is complete when apiclient_config_ is valid
  if (!apiclient_config_ || apiclient_config_->GetBaseUrl().empty()) {
    AquireInitLock();
  }

  auto cancel_context = std::make_shared<CancellationContext>();
  auto self = shared_from_this();

  cancel_context->ExecuteOrCancelled(
      [=]() -> CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<ApiError> init_api_error) {
              if (init_api_error) {
                NotifyInitAborted();
                callback(PublishSdiiResponse(init_api_error.get()));
                return;
              }

              NotifyInitCompleted();

              cancel_context->ExecuteOrCancelled(
                  [=]() -> CancellationToken {
                    return IngestApi::IngestSDII(
                        *self->apiclient_ingest_, request.GetLayerId(),
                        request.GetSdiiMessageList(), request.GetTraceId(),
                        request.GetBillingTag(), request.GetChecksum(),
                        [callback](IngestSdiiResponse response) {
                          callback(std::move(response));
                        });
                  },
                  [callback]() {
                    callback(PublishSdiiResponse(ApiError(
                        ErrorCode::Cancelled, "Operation cancelled.", true)));
                  });
            });
        ;
      },
      [callback]() {
        callback(PublishSdiiResponse(
            ApiError(ErrorCode::Cancelled, "Operation cancelled.", true)));
      });

  std::weak_ptr<StreamLayerClientImpl> weak_self{self};
  return CancellationToken([cancel_context, weak_self]() {
    cancel_context->CancelOperation();
    if (auto strong_self = weak_self.lock()) {
      strong_self->NotifyInitAborted();
    }
  });
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
