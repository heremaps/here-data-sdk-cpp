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

#include "VolatileLayerClientImpl.h"

#include <boost/format.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <olp/core/client/CancellationContext.h>

#include "ApiClientLookup.h"
#include "Common.h"
#include "generated/BlobApi.h"
#include "generated/ConfigApi.h"
#include "generated/MetadataApi.h"
#include "generated/PublishApi.h"
#include "generated/QueryApi.h"

namespace {
std::string GenerateUuid() {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}
}  // namespace

namespace olp {
namespace dataservice {
namespace write {
VolatileLayerClientImpl::VolatileLayerClientImpl(
    client::HRN catalog, client::OlpClientSettings settings)
    : catalog_(catalog),
      settings_(settings),
      catalog_settings_(catalog, settings),
      pending_requests_(std::make_shared<client::PendingRequests>()),
      task_scheduler_(settings.task_scheduler) {}

VolatileLayerClientImpl::~VolatileLayerClientImpl() {
  tokenList_.CancelAll();
  pending_requests_->CancelAllAndWait();
}

client::CancellationToken VolatileLayerClientImpl::InitApiClients(
    std::shared_ptr<client::CancellationContext> cancel_context,
    InitApiClientsCallback callback) {
  auto self = shared_from_this();
  std::unique_lock<std::mutex> ul{mutex_, std::try_to_lock};
  if (init_in_progress_) {
    ul.unlock();
    cond_var_.wait(ul, [self]() { return !self->init_in_progress_; });
    ul.lock();
  }
  if (apiclient_publish_ && !apiclient_publish_->GetBaseUrl().empty()) {
    callback(boost::none);
    return {};
  }

  init_in_progress_ = true;

  auto cancel_function = [callback]() {
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  apiclient_blob_ = client::OlpClientFactory::Create(settings_);
  apiclient_config_ = client::OlpClientFactory::Create(settings_);
  apiclient_metadata_ = client::OlpClientFactory::Create(settings_);
  apiclient_publish_ = client::OlpClientFactory::Create(settings_);
  apiclient_query_ = client::OlpClientFactory::Create(settings_);

  auto publishApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    std::lock_guard<std::mutex> lg{self->mutex_};
    init_in_progress_ = false;
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      self->cond_var_.notify_one();
    } else {
      self->apiclient_publish_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());
      callback(boost::none);
      self->cond_var_.notify_all();
    }
  };

  auto publishApi_function = [=]() -> client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_publish_, "publish", "v2",
                                      self->catalog_, publishApi_callback);
  };

  auto queryApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      std::lock_guard<std::mutex> lg{self->mutex_};
      init_in_progress_ = false;
      self->cond_var_.notify_one();
      return;
    }
    self->apiclient_query_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());

    cancel_context->ExecuteOrCancelled(publishApi_function, cancel_function);
  };

  auto queryApi_function = [=]() -> client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_query_, "query", "v1",
                                      self->catalog_, queryApi_callback);
  };

  auto blobApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      std::lock_guard<std::mutex> lg{self->mutex_};
      init_in_progress_ = false;
      self->cond_var_.notify_one();
      return;
    }

    self->apiclient_blob_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());

    cancel_context->ExecuteOrCancelled(queryApi_function, cancel_function);
  };

  auto blobApi_function = [=]() -> client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_blob_, "volatile-blob",
                                      "v1", self->catalog_, blobApi_callback);
  };

  auto metadataApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      std::lock_guard<std::mutex> lg{self->mutex_};
      init_in_progress_ = false;
      self->cond_var_.notify_one();
      return;
    }
    self->apiclient_metadata_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());

    cancel_context->ExecuteOrCancelled(blobApi_function, cancel_function);
  };

  auto metadataApi_function = [=]() -> client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_metadata_, "metadata",
                                      "v1", self->catalog_,
                                      metadataApi_callback);
  };

  ul.unlock();

  return ApiClientLookup::LookupApi(apiclient_metadata_, "metadata", "v1",
                                    self->catalog_, metadataApi_callback);
}

void VolatileLayerClientImpl::CancelPendingRequests() {
  tokenList_.CancelAll();
  pending_requests_->CancelAll();
}

client::CancellableFuture<GetBaseVersionResponse>
VolatileLayerClientImpl::GetBaseVersion() {
  auto promise = std::make_shared<std::promise<GetBaseVersionResponse>>();
  return client::CancellableFuture<GetBaseVersionResponse>(
      GetBaseVersion([promise](GetBaseVersionResponse response) {
        promise->set_value(response);
      }),
      promise);
}

client::CancellationToken VolatileLayerClientImpl::GetBaseVersion(
    GetBaseVersionCallback callback) {
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto self = shared_from_this();
  auto id = tokenList_.GetNextId();

  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(id);
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  auto getBaseVersion_callback =
      [=](MetadataApi::CatalogVersionResponse response) {
        self->tokenList_.RemoveTask(id);
        if (!response.IsSuccessful()) {
          if (response.GetError().GetHttpStatusCode() ==
                  http::HttpStatusCode::NOT_FOUND &&
              response.GetError().GetMessage().find(
                  "Catalog has no versions") != std::string::npos) {
            callback(GetBaseVersionResult{});
          } else {
            callback(std::move(response.GetError()));
          }
        } else {
          callback(response.MoveResult());
        }
      };

  auto getBaseVersion_function = [=]() -> client::CancellationToken {
    return MetadataApi::GetLatestCatalogVersion(
        *self->apiclient_metadata_, -1, boost::none, getBaseVersion_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> api_error) {
              if (api_error) {
                self->tokenList_.RemoveTask(id);
                callback(GetBaseVersionResponse(api_error.get()));
                return;
              }
              cancel_context->ExecuteOrCancelled(getBaseVersion_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto ret = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, ret);
  return ret;
}

client::CancellableFuture<StartBatchResponse>
VolatileLayerClientImpl::StartBatch(const model::StartBatchRequest& request) {
  auto promise = std::make_shared<std::promise<StartBatchResponse>>();
  return client::CancellableFuture<StartBatchResponse>(
      StartBatch(request,
                 [promise](StartBatchResponse response) {
                   promise->set_value(response);
                 }),
      promise);
}

client::CancellationToken VolatileLayerClientImpl::StartBatch(
    const model::StartBatchRequest& request, StartBatchCallback callback) {
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto self = shared_from_this();
  auto id = tokenList_.GetNextId();

  auto init_publication_callback =
      [=](InitPublicationResponse init_pub_response) {
        self->tokenList_.RemoveTask(id);
        if (!init_pub_response.IsSuccessful() ||
            !init_pub_response.GetResult().GetId()) {
          callback(std::move(init_pub_response.GetError()));
        } else {
          callback(init_pub_response.MoveResult());
        }
      };

  auto init_publication_function = [=]() -> client::CancellationToken {
    model::Publication pub;
    pub.SetLayerIds(request.GetLayers().value_or(std::vector<std::string>()));
    pub.SetVersionDependencies(request.GetVersionDependencies().value_or(
        std::vector<model::VersionDependency>()));

    return PublishApi::InitPublication(*self->apiclient_publish_, pub,
                                       request.GetBillingTag(),
                                       init_publication_callback);
  };

  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(id);
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> api_error) {
              if (api_error) {
                self->tokenList_.RemoveTask(id);
                callback(StartBatchResponse(api_error.get()));
                return;
              }
              cancel_context->ExecuteOrCancelled(init_publication_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, token);
  return token;
}

client::CancellableFuture<PublishPartitionDataResponse>
VolatileLayerClientImpl::PublishPartitionData(
    const model::PublishPartitionDataRequest& request) {
  auto promise = std::make_shared<std::promise<PublishPartitionDataResponse>>();
  auto cancel_token = PublishPartitionData(
      request, [promise](PublishPartitionDataResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<PublishPartitionDataResponse>(cancel_token,
                                                                 promise);
}

client::CancellationToken VolatileLayerClientImpl::PublishPartitionData(
    const model::PublishPartitionDataRequest& request,
    PublishPartitionDataCallback callback) {
  if (!request.GetData() || !request.GetPartitionId()) {
    callback(PublishPartitionDataResponse(
        client::ApiError(client::ErrorCode::InvalidArgument,
                         "Request data or partition id is not defined.")));
    return client::CancellationToken();
  }

  auto publish_task =
      [=](client::CancellationContext context) -> PublishPartitionDataResponse {
    auto partition_id = request.GetPartitionId().get();

    auto data_handle_response =
        GetDataHandleMap(request.GetLayerId(),
                         std::vector<std::string>{
                             request.GetPartitionId().value_or(std::string())},
                         boost::none, boost::none, boost::none, context);

    if (!data_handle_response.IsSuccessful()) {
      return data_handle_response.GetError();
    }

    if (data_handle_response.GetResult().empty()) {
      return client::ApiError(
          client::ErrorCode::InvalidArgument,
          "Unable to find requested partition,the partition "
          "metadata has to exist in OLP before invoking this API.");
    }

    auto data_handle_it = data_handle_response.GetResult().find(partition_id);

    if (data_handle_it == data_handle_response.GetResult().end()) {
      return client::ApiError(
          client::ErrorCode::Unknown,
          "Unexpected error. Partition data handle not found.");
    }
    const std::string& data_handle = data_handle_it->second;

    auto layer_settings_response = catalog_settings_.GetLayerSettings(
        context, request.GetBillingTag(), request.GetLayerId());
    if (!layer_settings_response.IsSuccessful()) {
      return layer_settings_response.GetError();
    }
    if (layer_settings_response.GetResult().content_type.empty()) {
      auto errmsg = boost::format(
                        "Unable to find the Layer ID (%1%) "
                        "provided in the PublishPartitionDataRequest "
                        "in the Catalog specified when creating "
                        "this VolatileLayerClient instance.") %
                    request.GetLayerId();
      return client::ApiError(client::ErrorCode::InvalidArgument, errmsg.str());
    }

    auto layer_settings = layer_settings_response.GetResult();

    auto blob_client_response = ApiClientLookup::LookupApiClient(
        catalog_, context, "volatile-blob", "v1", settings_);

    if (!blob_client_response.IsSuccessful()) {
      return blob_client_response.GetError();
    }

    auto blob_response = BlobApi::PutBlob(
        blob_client_response.GetResult(), request.GetLayerId(),
        layer_settings.content_type, layer_settings.content_encoding,
        data_handle, request.GetData(), request.GetBillingTag(), context);
    if (!blob_response.IsSuccessful()) {
      return blob_response.GetError();
    }

    model::ResponseOkSingle response_ok_single;
    response_ok_single.SetTraceID(partition_id);
    return response_ok_single;
  };

  return AddTask(task_scheduler_, pending_requests_, std::move(publish_task),
                 std::move(callback));
}

client::CancellableFuture<GetBatchResponse> VolatileLayerClientImpl::GetBatch(
    const model::Publication& pub) {
  auto promise = std::make_shared<std::promise<GetBatchResponse>>();
  return client::CancellableFuture<GetBatchResponse>(
      GetBatch(pub,
               [promise](GetBatchResponse response) {
                 promise->set_value(response);
               }),
      promise);
}

client::CancellationToken VolatileLayerClientImpl::GetBatch(
    const model::Publication& pub, GetBatchCallback callback) {
  std::string publicationId = pub.GetId().value_or("");
  if (publicationId.empty()) {
    callback(client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid publication", true));
    return {};
  }

  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto self = shared_from_this();
  auto id = tokenList_.GetNextId();

  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(id);
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  auto getPublication_callback =
      [=](GetPublicationResponse getPublicationResponse) {
        self->tokenList_.RemoveTask(id);
        if (!getPublicationResponse.IsSuccessful()) {
          callback(std::move(getPublicationResponse.GetError()));
        } else {
          callback(getPublicationResponse.MoveResult());
        }
      };

  auto getPublication_function = [=]() -> client::CancellationToken {
    return PublishApi::GetPublication(*self->apiclient_publish_, publicationId,
                                      boost::none, getPublication_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context,
            [=](boost::optional<client::ApiError> init_api_error) {
              if (init_api_error) {
                self->tokenList_.RemoveTask(id);
                callback(GetPublicationResponse(init_api_error.get()));
                return;
              }
              cancel_context->ExecuteOrCancelled(getPublication_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto ret = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, ret);
  return ret;
}

DataHandleMapResponse VolatileLayerClientImpl::GetDataHandleMap(
    const std::string& layer_id, const std::vector<std::string>& partition_ids,
    boost::optional<int64_t> version,
    boost::optional<std::vector<std::string>> additional_fields,
    boost::optional<std::string> billing_tag,
    client::CancellationContext context) {
  if (partition_ids.empty() || layer_id.empty()) {
    return client::ApiError(client::ErrorCode::InvalidArgument,
                            "Empty partition ids or layer id", true);
  }

  if (partition_ids.size() > 100) {
    return client::ApiError(
        client::ErrorCode::InvalidArgument,
        "Exceeds the maximum allowed number of partition per call", true);
  }

  auto api_response = ApiClientLookup::LookupApiClient(
      catalog_, context, "query", "v1", settings_);
  if (!api_response.IsSuccessful()) {
    return api_response.GetError();
  }

  auto partitions_response = QueryApi::GetPartitionsById(
      api_response.GetResult(), layer_id, partition_ids, version,
      additional_fields, billing_tag, context);

  if (!partitions_response.IsSuccessful()) {
    return partitions_response.GetError();
  }

  const auto& partitions = partitions_response.GetResult().GetPartitions();
  std::map<std::string, std::string> data_handle_map;
  for (const model::Partition& p : partitions) {
    if (std::find(partition_ids.begin(), partition_ids.end(),
                  p.GetPartition()) != partition_ids.end()) {
      data_handle_map[p.GetPartition()] = p.GetDataHandle();
    }
  }
  return data_handle_map;
}

client::CancellableFuture<PublishToBatchResponse>
VolatileLayerClientImpl::PublishToBatch(
    const model::Publication& pub,
    const std::vector<model::PublishPartitionDataRequest>& partitions) {
  auto promise = std::make_shared<std::promise<PublishToBatchResponse>>();
  return client::CancellableFuture<PublishToBatchResponse>(
      PublishToBatch(pub, partitions,
                     [promise](PublishToBatchResponse response) {
                       promise->set_value(response);
                     }),
      promise);
}

client::CancellationToken VolatileLayerClientImpl::PublishToBatch(
    const model::Publication& pub,
    const std::vector<model::PublishPartitionDataRequest>& partitions,
    PublishToBatchCallback callback) {
  std::string publication_id = pub.GetId().value_or("");
  if (publication_id.empty()) {
    callback(client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid publication - missing ID", true));
    return {};
  }

  if (partitions.empty()) {
    // Callback return
    callback(client::ApiError(
        client::ErrorCode::InvalidArgument,
        "PublishPartitionDataRequest list provided is empty", true));
    return {};
  }

  const auto first_layer_id = partitions[0].GetLayerId();
  const auto first_layer_billing_tag = partitions[0].GetBillingTag();
  for (auto& partition : partitions) {
    if (partition.GetLayerId().empty() ||
        partition.GetLayerId() != first_layer_id) {
      callback(client::ApiError(
          client::ErrorCode::InvalidArgument,
          "A PublishPartitionDataRequest provided does not specify a layer, or "
          "it is different from other layers in the list.",
          true));
      return {};
    }

    if (partition.GetData()) {
      callback(client::ApiError(
          client::ErrorCode::InvalidArgument,
          "PublishPartitionDataRequest contains data. This request is for "
          "publishing metadata only, please see the documentation.",
          true));
      return {};
    }

    if (!partition.GetPartitionId() || (*partition.GetPartitionId()).empty()) {
      callback(client::ApiError(client::ErrorCode::InvalidArgument,
                                "A PublishPartitionDataRequest in the list "
                                "does not specify a PartitionId",
                                true));
      return {};
    }
  }

  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto self = shared_from_this();
  auto id = tokenList_.GetNextId();

  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(id);
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  auto upload_partitions_callback =
      [=](UploadPartitionsResponse upload_partitions_response) {
        self->tokenList_.RemoveTask(id);
        if (!upload_partitions_response.IsSuccessful()) {
          callback(std::move(upload_partitions_response.GetError()));
        } else {
          callback(upload_partitions_response.MoveResult());
        }
      };

  auto upload_partitions_function = [=]() -> client::CancellationToken {
    std::vector<model::PublishPartition> pub_partition_list;
    for (const auto& partition_request : partitions) {
      model::PublishPartition pub_partition;
      pub_partition.SetPartition(
          partition_request.GetPartitionId().value_or(""));
      pub_partition.SetDataHandle(GenerateUuid());
      if (partition_request.GetChecksum()) {
        pub_partition.SetDataHandle(*partition_request.GetChecksum());
      }
      pub_partition_list.push_back(std::move(pub_partition));
    }

    model::PublishPartitions publish_partitions;
    publish_partitions.SetPartitions(std::move(pub_partition_list));

    return PublishApi::UploadPartitions(
        *self->apiclient_publish_, std::move(publish_partitions),
        publication_id, first_layer_id, first_layer_billing_tag,
        upload_partitions_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> api_error) {
              if (api_error) {
                self->tokenList_.RemoveTask(id);
                callback(api_error.get());
                return;
              }
              cancel_context->ExecuteOrCancelled(upload_partitions_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto ret = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, ret);
  return ret;
}

client::CancellableFuture<CompleteBatchResponse>
VolatileLayerClientImpl::CompleteBatch(const model::Publication& pub) {
  auto promise = std::make_shared<std::promise<CompleteBatchResponse>>();
  return client::CancellableFuture<CompleteBatchResponse>(
      CompleteBatch(pub,
                    [promise](CompleteBatchResponse response) {
                      promise->set_value(response);
                    }),
      promise);
}

client::CancellationToken VolatileLayerClientImpl::CompleteBatch(
    const model::Publication& pub, CompleteBatchCallback callback) {
  std::string publicationId = pub.GetId().value_or("");
  if (publicationId.empty()) {
    callback(client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid publication", true));
    return {};
  }

  auto self = shared_from_this();
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto id = tokenList_.GetNextId();
  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(id);
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  auto completePublication_callback =
      [=](SubmitPublicationResponse submitPublicationResponse) {
        self->tokenList_.RemoveTask(id);
        if (!submitPublicationResponse.IsSuccessful()) {
          callback(std::move(submitPublicationResponse.GetError()));
        } else {
          callback(submitPublicationResponse.MoveResult());
        }
      };

  auto completePublication_function = [=]() -> client::CancellationToken {
    return PublishApi::SubmitPublication(*self->apiclient_publish_,
                                         publicationId, boost::none,
                                         completePublication_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> api_error) {
              if (api_error) {
                self->tokenList_.RemoveTask(id);
                callback(CompleteBatchResponse(api_error.get()));
                return;
              }
              cancel_context->ExecuteOrCancelled(completePublication_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto ret = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, ret);
  return ret;
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
