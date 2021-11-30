/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include "VersionedLayerClientImpl.h"

#include <olp/core/client/OlpClientFactory.h>

#include "ApiClientLookup.h"
#include "Common.h"
#include "generated/BlobApi.h"
#include "generated/ConfigApi.h"
#include "generated/MetadataApi.h"
#include "generated/PublishApi.h"
#include "generated/QueryApi.h"

#include <boost/format.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace {
std::string GenerateUuid() {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}
}  // namespace

namespace olp {
namespace dataservice {
namespace write {

VersionedLayerClientImpl::VersionedLayerClientImpl(
    client::HRN catalog, client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      settings_(std::move(settings)),
      catalog_settings_(catalog_, settings_),
      apiclient_blob_(nullptr),
      apiclient_config_(nullptr),
      apiclient_metadata_(nullptr),
      apiclient_publish_(nullptr),
      apiclient_query_(nullptr),
      pending_requests_(std::make_shared<client::PendingRequests>()),
      init_in_progress_(false) {}

VersionedLayerClientImpl::~VersionedLayerClientImpl() {
  tokenList_.CancelAll();
  pending_requests_->CancelAllAndWait();
}

olp::client::CancellationToken VersionedLayerClientImpl::InitApiClients(
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

  auto publishApi_function = [=]() -> olp::client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_publish_, "publish", "v2",
                                      self->catalog_, publishApi_callback);
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

    cancel_context->ExecuteOrCancelled(publishApi_function, cancel_function);
  };

  auto blobApi_function = [=]() -> olp::client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_blob_, "blob", "v1",
                                      self->catalog_, blobApi_callback);
  };

  auto configApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      std::lock_guard<std::mutex> lg{self->mutex_};
      init_in_progress_ = false;
      self->cond_var_.notify_one();
      return;
    }
    self->apiclient_config_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());

    cancel_context->ExecuteOrCancelled(blobApi_function, cancel_function);
  };

  auto configApi_function = [=]() -> olp::client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_config_, "config", "v1",
                                      self->catalog_, configApi_callback);
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

    cancel_context->ExecuteOrCancelled(configApi_function, cancel_function);
  };

  ul.unlock();

  return ApiClientLookup::LookupApi(apiclient_metadata_, "metadata", "v1",
                                    catalog_, metadataApi_callback);
}

client::CancellableFuture<StartBatchResponse>
VersionedLayerClientImpl::StartBatch(const model::StartBatchRequest& request) {
  auto promise = std::make_shared<std::promise<StartBatchResponse>>();
  return olp::client::CancellableFuture<StartBatchResponse>(
      StartBatch(request,
                 [promise](StartBatchResponse response) {
                   promise->set_value(response);
                 }),
      promise);
}

client::CancellationToken VersionedLayerClientImpl::StartBatch(
    const model::StartBatchRequest& request, StartBatchCallback callback) {
  const auto catalog = catalog_;
  const auto settings = settings_;

  auto start_batch_task =
      [=](client::CancellationContext context) -> StartBatchResponse {
    if (!request.GetLayers() || request.GetLayers()->empty()) {
      return {{client::ErrorCode::InvalidArgument, "Invalid layer", true}};
    }

    auto olp_client_response = ApiClientLookup::LookupApiClient(
        catalog, context, "publish", "v2", settings);

    if (!olp_client_response.IsSuccessful()) {
      return olp_client_response.GetError();
    }

    auto olp_client = olp_client_response.MoveResult();
    model::Publication pub;
    pub.SetLayerIds(request.GetLayers().value_or(std::vector<std::string>()));
    pub.SetVersionDependencies(request.GetVersionDependencies().value_or(
        std::vector<model::VersionDependency>()));

    return PublishApi::InitPublication(olp_client, pub, request.GetBillingTag(),
                                       context);
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(start_batch_task), std::move(callback));
}

olp::client::CancellableFuture<GetBaseVersionResponse>
VersionedLayerClientImpl::GetBaseVersion() {
  auto promise = std::make_shared<std::promise<GetBaseVersionResponse>>();
  return olp::client::CancellableFuture<GetBaseVersionResponse>(
      GetBaseVersion([promise](GetBaseVersionResponse response) {
        promise->set_value(response);
      }),
      promise);
}

olp::client::CancellationToken VersionedLayerClientImpl::GetBaseVersion(
    GetBaseVersionCallback callback) {
  auto self = shared_from_this();
  auto cancel_context = std::make_shared<client::CancellationContext>();
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
            cancel_context, [=](boost::optional<client::ApiError> err) {
              if (err) {
                self->tokenList_.RemoveTask(id);
                callback(err.get());
                return;
              }
              cancel_context->ExecuteOrCancelled(getBaseVersion_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, token);
  return token;
}

olp::client::CancellableFuture<GetBatchResponse>
VersionedLayerClientImpl::GetBatch(const model::Publication& pub) {
  auto promise = std::make_shared<std::promise<GetBatchResponse>>();
  return olp::client::CancellableFuture<GetBatchResponse>(
      GetBatch(pub,
               [promise](GetBatchResponse response) {
                 promise->set_value(response);
               }),
      promise);
}

olp::client::CancellationToken VersionedLayerClientImpl::GetBatch(
    const model::Publication& pub, GetBatchCallback callback) {
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
            cancel_context, [=](boost::optional<client::ApiError> err) {
              if (err) {
                self->tokenList_.RemoveTask(id);
                callback(err.get());
                return;
              }
              cancel_context->ExecuteOrCancelled(getPublication_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, token);
  return token;
}

olp::client::CancellableFuture<CompleteBatchResponse>
VersionedLayerClientImpl::CompleteBatch(const model::Publication& pub) {
  auto promise = std::make_shared<std::promise<CompleteBatchResponse>>();
  return olp::client::CancellableFuture<CompleteBatchResponse>(
      CompleteBatch(pub,
                    [promise](CompleteBatchResponse response) {
                      promise->set_value(response);
                    }),
      promise);
}

olp::client::CancellationToken VersionedLayerClientImpl::CompleteBatch(
    const model::Publication& publication, CompleteBatchCallback callback) {
  const auto catalog = catalog_;
  const auto settings = settings_;

  auto task =
      [=](client::CancellationContext context) -> CompleteBatchResponse {
    if (!publication.GetId()) {
      return {{client::ErrorCode::InvalidArgument, "Invalid publication"}};
    }
    const auto& id = publication.GetId().get();

    auto olp_client_response = ApiClientLookup::LookupApiClient(
        catalog, context, "publish", "v2", settings);
    if (!olp_client_response.IsSuccessful()) {
      return olp_client_response.GetError();
    }

    auto olp_client = olp_client_response.MoveResult();

    return PublishApi::SubmitPublication(olp_client, id, boost::none, context);
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 std::move(callback));
}

olp::client::CancellableFuture<CancelBatchResponse>
VersionedLayerClientImpl::CancelBatch(const model::Publication& publication) {
  auto promise = std::make_shared<std::promise<CancelBatchResponse>>();
  return olp::client::CancellableFuture<CancelBatchResponse>(
      CancelBatch(publication,
                  [promise](CancelBatchResponse response) {
                    promise->set_value(response);
                  }),
      promise);
}

olp::client::CancellationToken VersionedLayerClientImpl::CancelBatch(
    const model::Publication& publication, CancelBatchCallback callback) {
  auto cancel_batch_task =
      [=](client::CancellationContext context) -> CancelBatchResponse {
    const auto publication_id = publication.GetId().value_or("");
    if (publication_id.empty()) {
      return client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid publication: publication ID missing",
                              true);
    }

    auto client_response = ApiClientLookup::LookupApiClient(
        catalog_, context, "publish", "v2", settings_);
    if (!client_response.IsSuccessful()) {
      return client_response.GetError();
    }

    return PublishApi::CancelPublication(client_response.GetResult(),
                                         publication_id, boost::none, context);
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(cancel_batch_task), std::move(callback));
}

void VersionedLayerClientImpl::CancelPendingRequests() {
  pending_requests_->CancelAll();
  tokenList_.CancelAll();
}

olp::client::CancellableFuture<PublishPartitionDataResponse>
VersionedLayerClientImpl::PublishToBatch(
    const model::Publication& pub,
    const model::PublishPartitionDataRequest& request) {
  auto promise = std::make_shared<std::promise<PublishPartitionDataResponse>>();
  return olp::client::CancellableFuture<PublishPartitionDataResponse>(
      PublishToBatch(pub, request,
                     [promise](PublishPartitionDataResponse response) {
                       promise->set_value(response);
                     }),
      promise);
}

olp::client::CancellationToken VersionedLayerClientImpl::PublishToBatch(
    const model::Publication& pub,
    const model::PublishPartitionDataRequest& request,
    PublishPartitionDataCallback callback) {
  auto publish_task =
      [=](client::CancellationContext context) -> PublishPartitionDataResponse {
    if (!pub.GetId()) {
      return {{client::ErrorCode::InvalidArgument,
               "Invalid publication: publication ID missing", true}};
    }
    const auto& publication_id = pub.GetId().get();

    const auto& layer_id = request.GetLayerId();
    if (layer_id.empty()) {
      return {{client::ErrorCode::InvalidArgument,
               "Invalid publication: layer ID missing", true}};
    }

    const auto data_handle = GenerateUuid();
    model::PublishPartition partition;
    partition.SetPartition(request.GetPartitionId().value_or(""));
    partition.SetData(request.GetData());
    partition.SetDataHandle(data_handle);

    auto layer_settings_response = catalog_settings_.GetLayerSettings(
        context, request.GetBillingTag(), layer_id);
    if (!layer_settings_response.IsSuccessful()) {
      return layer_settings_response.GetError();
    }

    auto layer_settings = layer_settings_response.GetResult();
    if (layer_settings.content_type.empty()) {
      auto errmsg = boost::format(
                        "Unable to find the Layer ID (%1%) "
                        "provided in the request in the "
                        "Catalog specified when creating "
                        "this VersionedLayerClient instance.") %
                    layer_id;
      return {{client::ErrorCode::InvalidArgument, errmsg.str()}};
    }

    auto upload_blob_response =
        UploadBlob(partition, data_handle, layer_settings.content_type,
                   layer_settings.content_encoding, layer_id,
                   request.GetBillingTag(), context);
    if (!upload_blob_response.IsSuccessful()) {
      return upload_blob_response.GetError();
    }

    auto upload_partition_response =
        UploadPartition(publication_id, partition, layer_id, context);
    if (!upload_partition_response.IsSuccessful()) {
      return upload_partition_response.GetError();
    }

    model::ResponseOkSingle res;
    res.SetTraceID(partition.GetPartition().value_or(""));
    return res;
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(publish_task), std::move(callback));
}

UploadPartitionResponse VersionedLayerClientImpl::UploadPartition(
    const std::string& publication_id, const model::PublishPartition& partition,
    const std::string& layer_id, client::CancellationContext context) {
  auto olp_client_response = ApiClientLookup::LookupApiClient(
      catalog_, context, "publish", "v2", settings_);
  if (!olp_client_response.IsSuccessful()) {
    return olp_client_response.GetError();
  }

  auto publish_client = olp_client_response.MoveResult();

  model::PublishPartition publish_partition;
  publish_partition.SetPartition(partition.GetPartition().value_or(""));
  publish_partition.SetDataHandle(partition.GetDataHandle().value_or(""));
  model::PublishPartitions partitions;
  partitions.SetPartitions({publish_partition});

  return PublishApi::UploadPartitions(publish_client, partitions,
                                      publication_id, layer_id, boost::none,
                                      context);
}

UploadBlobResponse VersionedLayerClientImpl::UploadBlob(
    const model::PublishPartition& partition, const std::string& data_handle,
    const std::string& content_type, const std::string& content_encoding,
    const std::string& layer_id, BillingTag billing_tag,
    client::CancellationContext context) {
  auto olp_client_response = ApiClientLookup::LookupApiClient(
      catalog_, context, "blob", "v1", settings_);
  if (!olp_client_response.IsSuccessful()) {
    return olp_client_response.GetError();
  }

  auto blob_client = olp_client_response.MoveResult();
  return BlobApi::PutBlob(blob_client, layer_id, content_type, content_encoding,
                          data_handle, partition.GetData(), billing_tag,
                          context);
}

client::CancellableFuture<CheckDataExistsResponse>
VersionedLayerClientImpl::CheckDataExists(
    const model::CheckDataExistsRequest& request) {
  auto promise = std::make_shared<std::promise<CheckDataExistsResponse>>();
  return olp::client::CancellableFuture<CheckDataExistsResponse>(
      CheckDataExists(request,
                      [promise](CheckDataExistsResponse response) {
                        promise->set_value(response);
                      }),
      promise);
}

client::CancellationToken VersionedLayerClientImpl::CheckDataExists(
    const model::CheckDataExistsRequest& request,
    CheckDataExistsCallback callback) {
  if (request.GetLayerId().empty()) {
    callback(client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid layer", true));
    return {};
  }

  auto layer_id = request.GetLayerId();
  auto data_handle = request.GetDataHandle();

  auto self = shared_from_this();
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto id = tokenList_.GetNextId();

  auto check_data_exists_callback =
      [=](CheckDataExistsResponse check_data_exists_response) {
        self->tokenList_.RemoveTask(id);
        if (!check_data_exists_response.IsSuccessful()) {
          callback(std::move(check_data_exists_response.GetError()));
        } else {
          callback(check_data_exists_response.MoveResult());
        }
      };

  auto check_data_exists_function = [=]() -> client::CancellationToken {
    return BlobApi::checkBlobExists(*self->apiclient_blob_, layer_id,
                                    data_handle, boost::none,
                                    check_data_exists_callback);
  };

  auto cancel_function = [callback]() {
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> err) {
              if (err) {
                callback(err.get());
                return;
              }
              cancel_context->ExecuteOrCancelled(check_data_exists_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, token);
  return token;
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
