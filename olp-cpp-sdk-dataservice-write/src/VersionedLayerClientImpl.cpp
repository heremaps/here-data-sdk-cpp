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

#include "VersionedLayerClientImpl.h"

#include <olp/core/client/OlpClientFactory.h>

#include "ApiClientLookup.h"
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
    const client::HRN& catalog, const client::OlpClientSettings& settings)
    : catalog_(catalog),
      settings_(settings),
      apiclient_blob_(nullptr),
      apiclient_config_(nullptr),
      apiclient_metadata_(nullptr),
      apiclient_publish_(nullptr),
      apiclient_query_(nullptr),
      init_in_progress_(false) {}

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
    const model::StartBatchRequest& request,
    const StartBatchCallback& callback) {
  if (!request.GetLayers() || request.GetLayers()->empty()) {
    callback(client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid layer", true));
    return {};
  }

  auto self = shared_from_this();
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto id = tokenList_.GetNextId();

  auto init_publication_callback =
      [=](InitPublicationResponse init_pub_response) {
        self->tokenList_.RemoveTask(id);
        if (!init_pub_response.IsSuccessful() ||
            !init_pub_response.GetResult().GetId()) {
          callback(std::move(init_pub_response.GetError()));
        } else {
          callback(std::move(init_pub_response.GetResult()));
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
            cancel_context, [=](boost::optional<client::ApiError> err) {
              if (err) {
                self->tokenList_.RemoveTask(id);
                callback(err.get());
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
    const GetBaseVersionCallback& callback) {
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
          if (response.GetError().GetHttpStatusCode() == 404 &&
              response.GetError().GetMessage().find(
                  "Catalog has no versions") != std::string::npos) {
            callback(GetBaseVersionResult{});
          } else {
            callback(std::move(response.GetError()));
          }
        } else {
          callback(std::move(response.GetResult()));
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
    const model::Publication& pub, const GetBatchCallback& callback) {
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
          callback(std::move(getPublicationResponse.GetResult()));
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
    const model::Publication& pub, const CompleteBatchCallback& callback) {
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
          callback(std::move(submitPublicationResponse.GetResult()));
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
            cancel_context, [=](boost::optional<client::ApiError> err) {
              if (err) {
                self->tokenList_.RemoveTask(id);
                callback(err.get());
                return;
              }
              cancel_context->ExecuteOrCancelled(completePublication_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, token);
  return token;
}

olp::client::CancellableFuture<CancelBatchResponse>
VersionedLayerClientImpl::CancelBatch(const model::Publication& pub) {
  auto promise = std::make_shared<std::promise<CancelBatchResponse>>();
  return olp::client::CancellableFuture<CancelBatchResponse>(
      CancelBatch(pub,
                  [promise](CancelBatchResponse response) {
                    promise->set_value(response);
                  }),
      promise);
}

olp::client::CancellationToken VersionedLayerClientImpl::CancelBatch(
    const model::Publication& pub, const CancelBatchCallback& callback) {
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

  auto cancelPublication_callback =
      [=](CancelPublicationResponse cancelPublicationResponse) {
        self->tokenList_.RemoveTask(id);
        if (!cancelPublicationResponse.IsSuccessful()) {
          callback(std::move(cancelPublicationResponse.GetError()));
        } else {
          callback(std::move(cancelPublicationResponse.GetResult()));
        }
      };

  auto cancelPublication_function = [=]() -> client::CancellationToken {
    return PublishApi::CancelPublication(*self->apiclient_publish_,
                                         publicationId, boost::none,
                                         cancelPublication_callback);
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
              cancel_context->ExecuteOrCancelled(cancelPublication_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, token);
  return token;
}

void VersionedLayerClientImpl::CancelAll() { tokenList_.CancelAll(); }

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
    const PublishPartitionDataCallback& callback) {
  std::string publication_id = pub.GetId().value_or("");
  if (publication_id.empty()) {
    callback(client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid publication", true));
    return {};
  }

  std::string layer_id = request.GetLayerId();
  if (layer_id.empty()) {
    callback(client::ApiError(client::ErrorCode::InvalidArgument,
                              "Invalid request", true));
    return {};
  }

  const auto data_handle = GenerateUuid();
  std::shared_ptr<model::PublishPartition> partition =
      std::make_shared<model::PublishPartition>();
  partition->SetPartition(request.GetPartitionId().value_or(""));
  partition->SetData(request.GetData());
  partition->SetDataHandle(data_handle);

  auto self = shared_from_this();
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto id = tokenList_.GetNextId();
  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(id);
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  auto uploadPartition_callback = [=](UploadPartitionResponse response) {
    self->tokenList_.RemoveTask(id);
    if (!response.IsSuccessful()) {
      callback(std::move(response.GetError()));
    } else {
      model::ResponseOkSingle res;
      res.SetTraceID(partition->GetPartition().value_or(""));
      callback(std::move(res));
    }
  };

  auto uploadBlob_callback = [=](UploadBlobResponse response) {
    if (!response.IsSuccessful()) {
      self->tokenList_.RemoveTask(id);
      callback(std::move(response.GetError()));
    } else {
      self->UploadPartition(publication_id, partition, layer_id, cancel_context,
                            uploadPartition_callback);
    }
  };

  auto catalogModel_callback = [=](boost::optional<client::ApiError> error) {
    if (error) {
      self->tokenList_.RemoveTask(id);
      callback(std::move(*error));
    } else {
      self->UploadBlob(publication_id, partition, data_handle, layer_id,
                       cancel_context, uploadBlob_callback);
    }
  };

  InitCatalogModel(cancel_context, catalogModel_callback);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(id, token);
  return token;
}

void VersionedLayerClientImpl::InitCatalogModel(
    std::shared_ptr<client::CancellationContext> cancel_context,
    const InitCatalogModelCallback& callback) {
  auto self = shared_from_this();
  auto cancel_function = [callback]() {
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  auto initCatalog_callback = [=](CatalogResponse response) mutable {
    if (!response.IsSuccessful()) {
      callback(std::move(response.GetError()));
    } else {
      catalog_model_ = response.GetResult();
      callback(boost::none);
    }
  };

  auto initCatalog_function = [=]() -> client::CancellationToken {
    return ConfigApi::GetCatalog(self->apiclient_config_, catalog_.ToString(),
                                 boost::none, initCatalog_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> err) {
              if (err) {
                callback(err.get());
                return;
              }
              if (!self->catalog_model_.GetId().empty()) {
                callback(boost::none);
              } else {
                cancel_context->ExecuteOrCancelled(initCatalog_function,
                                                   cancel_function);
              }
            });
      },
      cancel_function);
}

void VersionedLayerClientImpl::UploadPartition(
    std::string publication_id,
    std::shared_ptr<model::PublishPartition> partition, std::string layer_id,
    std::shared_ptr<client::CancellationContext> cancel_context,
    const UploadPartitionCallback& callback) {
  auto self = shared_from_this();
  auto cancel_function = [callback]() {
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  std::shared_ptr<model::PublishPartition> publishPartition =
      std::make_shared<model::PublishPartition>();
  publishPartition->SetPartition(partition->GetPartition().value_or(""));
  publishPartition->SetDataHandle(partition->GetDataHandle().value_or(""));

  auto uploadPartition_callback = [=](UploadPartitionsResponse response) {
    if (!response.IsSuccessful()) {
      callback(std::move(response.GetError()));
    } else {
      callback(std::move(response.GetResult()));
    }
  };

  auto uploadPartition_function = [=]() -> client::CancellationToken {
    model::PublishPartitions partitions;
    partitions.SetPartitions({*publishPartition});
    return PublishApi::UploadPartitions(*self->apiclient_publish_, partitions,
                                        publication_id, layer_id, boost::none,
                                        uploadPartition_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> err) {
              if (err) {
                callback(err.get());
                return;
              }
              cancel_context->ExecuteOrCancelled(uploadPartition_function,
                                                 cancel_function);
            });
      },
      cancel_function);
}

std::string VersionedLayerClientImpl::FindContentTypeForLayerId(
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

void VersionedLayerClientImpl::UploadBlob(
    std::string publication_id,
    std::shared_ptr<model::PublishPartition> partition, std::string data_handle,
    std::string layer_id,
    std::shared_ptr<client::CancellationContext> cancel_context,
    const UploadBlobCallback& callback) {
  auto self = shared_from_this();
  auto cancel_function = [callback]() {
    callback(client::ApiError(client::ErrorCode::Cancelled,
                              "Operation cancelled.", true));
  };

  auto uploadBlob_callback = [=](UploadBlobResponse response) {
    if (!response.IsSuccessful()) {
      callback(std::move(response.GetError()));
    } else {
      callback(std::move(response.GetResult()));
    }
  };

  auto content_type = FindContentTypeForLayerId(layer_id);
  if (content_type.empty()) {
    auto errmsg = boost::format(
                      "Unable to find the Layer ID (%1%) "
                      "provided in the request in the "
                      "Catalog specified when creating "
                      "this VersionedLayerClient instance.") %
                  layer_id;
    callback(UploadBlobResponse(
        client::ApiError(client::ErrorCode::InvalidArgument, errmsg.str())));
    return;
  }

  auto uploadBlob_function = [=]() -> client::CancellationToken {
    return BlobApi::PutBlob(*self->apiclient_blob_, layer_id, content_type,
                            data_handle, partition->GetData(), boost::none,
                            uploadBlob_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
        return self->InitApiClients(cancel_context,
                                    [=](boost::optional<client::ApiError> err) {
                                      if (err) {
                                        callback(err.get());
                                        return;
                                      }
                                      cancel_context->ExecuteOrCancelled(
                                          uploadBlob_function, cancel_function);
                                    });
      },
      cancel_function);
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
    const CheckDataExistsCallback& callback) {
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
          callback(std::move(check_data_exists_response.GetResult()));
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
