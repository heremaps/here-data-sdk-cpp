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

#include "IndexLayerClientImpl.h"

#include <boost/format.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <olp/core/client/CancellationContext.h>

#include "ApiClientLookup.h"
#include "generated/BlobApi.h"
#include "generated/ConfigApi.h"
#include "generated/IndexApi.h"

#include <atomic>

using namespace olp::client;
using namespace olp::dataservice::write::model;

namespace {
std::string GenerateUuid() {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}
}  // namespace

namespace olp {
namespace dataservice {
namespace write {

IndexLayerClientImpl::IndexLayerClientImpl(HRN catalog,
                                           OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      catalog_model_(),
      settings_(std::move(settings)),
      apiclient_config_(nullptr),
      apiclient_blob_(nullptr),
      apiclient_index_(nullptr),
      init_in_progress_(false) {}

IndexLayerClientImpl::~IndexLayerClientImpl() { CancelAll(); }

olp::client::CancellationToken IndexLayerClientImpl::InitApiClients(
    std::shared_ptr<client::CancellationContext> cancel_context,
    InitApiClientsCallback callback) {
  auto self = shared_from_this();
  std::unique_lock<std::mutex> ul{mutex_, std::try_to_lock};
  if (init_in_progress_) {
    ul.unlock();
    cond_var_.wait(ul, [self]() { return !self->init_in_progress_; });
    ul.lock();
  }
  if (apiclient_index_ && !apiclient_index_->GetBaseUrl().empty()) {
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
  apiclient_index_ = client::OlpClientFactory::Create(settings_);

  auto indexApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    std::lock_guard<std::mutex> lg{self->mutex_};
    self->init_in_progress_ = false;
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      self->cond_var_.notify_one();
    } else {
      self->apiclient_index_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());
      callback(boost::none);
      self->cond_var_.notify_all();
    }
  };

  auto indexApi_function = [=]() -> olp::client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_index_, "index", "v1",
                                      self->catalog_, indexApi_callback);
  };

  auto blobApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      std::lock_guard<std::mutex> lg{self->mutex_};
      self->init_in_progress_ = false;
      self->cond_var_.notify_one();
      return;
    }

    self->apiclient_blob_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());

    cancel_context->ExecuteOrCancelled(indexApi_function, cancel_function);
  };

  auto blobApi_function = [=]() -> olp::client::CancellationToken {
    return ApiClientLookup::LookupApi(self->apiclient_blob_, "blob", "v1",
                                      self->catalog_, blobApi_callback);
  };

  auto configApi_callback = [=](ApiClientLookup::ApisResponse apis) {
    if (!apis.IsSuccessful()) {
      callback(std::move(apis.GetError()));
      std::lock_guard<std::mutex> lg{self->mutex_};
      self->init_in_progress_ = false;
      self->cond_var_.notify_one();
      return;
    }
    self->apiclient_config_->SetBaseUrl(apis.GetResult().at(0).GetBaseUrl());

    cancel_context->ExecuteOrCancelled(blobApi_function, cancel_function);
  };

  ul.unlock();

  return ApiClientLookup::LookupApi(apiclient_config_, "config", "v1", catalog_,
                                    configApi_callback);
}

void IndexLayerClientImpl::CancelAll() { tokenList_.CancelAll(); }

CancellationToken IndexLayerClientImpl::InitCatalogModel(
    const model::PublishIndexRequest& request,
    const InitCatalogModelCallback& callback) {
  if (!catalog_model_.GetId().empty()) {
    callback(boost::none);
    return CancellationToken();
  }

  auto self = shared_from_this();
  return ConfigApi::GetCatalog(
      apiclient_config_, catalog_.ToString(), boost::none,
      [=](CatalogResponse catalog_response) mutable {
        if (!catalog_response.IsSuccessful()) {
          callback(std::move(catalog_response.GetError()));
          return;
        }

        self->catalog_model_ = catalog_response.GetResult();

        callback(boost::none);
      });
}

std::string IndexLayerClientImpl::FindContentTypeForLayerId(
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

CancellableFuture<PublishIndexResponse> IndexLayerClientImpl::PublishIndex(
    const model::PublishIndexRequest& request) {
  auto promise = std::make_shared<std::promise<PublishIndexResponse> >();
  auto cancel_token =
      PublishIndex(request, [promise](PublishIndexResponse response) {
        promise->set_value(std::move(response));
      });
  return CancellableFuture<PublishIndexResponse>(cancel_token, promise);
}

CancellationToken IndexLayerClientImpl::PublishIndex(
    model::PublishIndexRequest request, const PublishIndexCallback& callback) {
  if (!request.GetData()) {
    callback(PublishIndexResponse(
        ApiError(ErrorCode::InvalidArgument, "Request data empty.")));
    return CancellationToken();
  }

  if (request.GetLayerId().empty()) {
    callback(PublishIndexResponse(
        ApiError(ErrorCode::InvalidArgument, "Request layer Id empty.")));
    return CancellationToken();
  }

  auto op_id = tokenList_.GetNextId();
  auto cancel_context = std::make_shared<CancellationContext>();
  auto self = shared_from_this();
  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(op_id);
    callback(PublishIndexResponse(
        ApiError(ErrorCode::Cancelled, "Operation cancelled.", true)));
  };

  const auto data_handle = GenerateUuid();

  auto insert_indexes_callback =
      [=](InsertIndexesResponse insert_indexes_response) {
        self->tokenList_.RemoveTask(op_id);
        if (!insert_indexes_response.IsSuccessful()) {
          callback(PublishIndexResponse(insert_indexes_response.GetError()));
          return;
        }
        model::ResponseOkSingle res;
        res.SetTraceID(data_handle);
        callback(PublishIndexResponse(std::move(res)));
      };

  auto insert_indexes_function = [=]() -> CancellationToken {
    auto index = request.GetIndex();
    index.SetId(data_handle);
    return IndexApi::insertIndexes(
        *self->apiclient_index_, index, request.GetLayerId(),
        request.GetBillingTag(), insert_indexes_callback);
  };

  auto put_blob_callback = [=](PutBlobResponse put_blob_response) {
    if (!put_blob_response.IsSuccessful()) {
      self->tokenList_.RemoveTask(op_id);
      callback(PublishIndexResponse(put_blob_response.GetError()));
      return;
    }

    cancel_context->ExecuteOrCancelled(insert_indexes_function,
                                       cancel_function);
  };

  auto put_blob_function = [=](std::string content_type) -> CancellationToken {
    return BlobApi::PutBlob(*self->apiclient_blob_, request.GetLayerId(),
                            content_type, data_handle, request.GetData(),
                            request.GetBillingTag(), put_blob_callback);
  };

  auto init_catalog_model_callback =
      [=](boost::optional<ApiError> init_catalog_model_error) {
        if (init_catalog_model_error) {
          self->tokenList_.RemoveTask(op_id);
          callback(PublishIndexResponse(init_catalog_model_error.get()));
          return;
        }
        auto content_type =
            self->FindContentTypeForLayerId(request.GetLayerId());
        if (content_type.empty()) {
          auto errmsg = boost::format(
                            "Unable to find the Layer ID (%1%) "
                            "provided in the PublishIndexRequest in the "
                            "Catalog specified when creating "
                            "this IndexLayerClient instance.") %
                        request.GetLayerId();
          callback(PublishIndexResponse(
              ApiError(ErrorCode::InvalidArgument, errmsg.str())));
          return;
        }

        cancel_context->ExecuteOrCancelled(
            std::bind(put_blob_function, content_type), cancel_function);
      };

  auto init_catalog_model_function = [=]() -> CancellationToken {
    return self->InitCatalogModel(request, init_catalog_model_callback);
  };

  auto init_api_client_callback =
      [=](boost::optional<ApiError> init_api_error) {
        if (init_api_error) {
          self->tokenList_.RemoveTask(op_id);
          callback(PublishIndexResponse(init_api_error.get()));
          return;
        }

        cancel_context->ExecuteOrCancelled(init_catalog_model_function,
                                           cancel_function);
      };

  auto init_api_client_function = [=]() -> CancellationToken {
    return self->InitApiClients(cancel_context, init_api_client_callback);
  };

  cancel_context->ExecuteOrCancelled(init_api_client_function, cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(op_id, token);
  return token;
}

CancellableFuture<DeleteIndexDataResponse>
IndexLayerClientImpl::DeleteIndexData(
    const model::DeleteIndexDataRequest& request) {
  auto promise = std::make_shared<std::promise<DeleteIndexDataResponse> >();
  auto cancel_token =
      DeleteIndexData(request, [promise](DeleteIndexDataResponse response) {
        promise->set_value(std::move(response));
      });
  return CancellableFuture<DeleteIndexDataResponse>(cancel_token, promise);
}

CancellationToken IndexLayerClientImpl::DeleteIndexData(
    const model::DeleteIndexDataRequest& request,
    const DeleteIndexDataCallback& callback) {
  if (request.GetLayerId().empty() || request.GetIndexId().empty()) {
    callback(DeleteIndexDataResponse(
        ApiError(ErrorCode::InvalidArgument,
                 "Request layer ID or Index Id is not defined.")));
    return CancellationToken();
  }

  auto layer_id = request.GetLayerId();
  auto index_id = request.GetIndexId();

  auto op_id = tokenList_.GetNextId();
  auto cancel_context = std::make_shared<CancellationContext>();
  auto self = shared_from_this();

  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(op_id);
    callback(DeleteIndexDataResponse(
        ApiError(ErrorCode::Cancelled, "Operation cancelled.", true)));
  };

  auto delete_index_data_function = [=]() -> client::CancellationToken {
    return BlobApi::deleteBlob(
        *self->apiclient_blob_, layer_id, index_id, boost::none,
        [=](DeleteBlobRespone response) {
          self->tokenList_.RemoveTask(op_id);
          if (!response.IsSuccessful()) {
            callback(DeleteBlobRespone(response.GetError()));
            return;
          }
          callback(DeleteBlobRespone(ApiNoResult()));
        });
  };

  auto init_api_client_callback =
      [=](boost::optional<ApiError> init_api_error) {
        if (init_api_error) {
          self->tokenList_.RemoveTask(op_id);
          callback(DeleteBlobRespone(init_api_error.get()));
          return;
        }

        cancel_context->ExecuteOrCancelled(delete_index_data_function,
                                           cancel_function);
      };

  auto init_api_client_function = [=]() -> CancellationToken {
    return self->InitApiClients(cancel_context, init_api_client_callback);
  };

  cancel_context->ExecuteOrCancelled(init_api_client_function, cancel_function);

  auto ret = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(op_id, ret);
  return ret;
}

CancellableFuture<UpdateIndexResponse> IndexLayerClientImpl::UpdateIndex(
    const model::UpdateIndexRequest& request) {
  auto promise = std::make_shared<std::promise<UpdateIndexResponse> >();
  auto cancel_token =
      UpdateIndex(request, [promise](UpdateIndexResponse response) {
        promise->set_value(std::move(response));
      });
  return CancellableFuture<UpdateIndexResponse>(cancel_token, promise);
}

CancellationToken IndexLayerClientImpl::UpdateIndex(
    const model::UpdateIndexRequest& request,
    const UpdateIndexCallback& callback) {
  auto cancel_context = std::make_shared<CancellationContext>();
  auto self = shared_from_this();

  auto op_id = tokenList_.GetNextId();
  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(op_id);
    callback(UpdateIndexResponse(
        ApiError(ErrorCode::Cancelled, "Operation cancelled.", true)));
  };

  auto updateIndex_callback = [=](UpdateIndexResponse update_index_response) {
    self->tokenList_.RemoveTask(op_id);
    if (!update_index_response.IsSuccessful()) {
      callback(UpdateIndexResponse(update_index_response.GetError()));
      return;
    }
    callback(UpdateIndexResponse(ApiNoResult()));
  };

  auto UpdateIndex_function = [=]() -> client::CancellationToken {
    return IndexApi::performUpdate(*self->apiclient_index_, request,
                                   boost::none, updateIndex_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> CancellationToken {
        return self->InitApiClients(
            cancel_context, [=](boost::optional<client::ApiError> api_error) {
              if (api_error) {
                self->tokenList_.RemoveTask(op_id);
                callback(UpdateIndexResponse(api_error.get()));
                return;
              }
              cancel_context->ExecuteOrCancelled(UpdateIndex_function,
                                                 cancel_function);
            });
      },
      cancel_function);

  auto token = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(op_id, token);
  return token;
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
