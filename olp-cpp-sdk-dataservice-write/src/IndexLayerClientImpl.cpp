/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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
#include "Common.h"
#include "generated/BlobApi.h"
#include "generated/ConfigApi.h"
#include "generated/IndexApi.h"

#include <atomic>
#include <string>
#include <utility>

namespace {
std::string GenerateUuid() {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}
}  // namespace

namespace olp {
namespace dataservice {
namespace write {

IndexLayerClientImpl::IndexLayerClientImpl(client::HRN catalog,
                                           client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      catalog_settings_(catalog_, settings),
      settings_(std::move(settings)),
      apiclient_config_(nullptr),
      apiclient_blob_(nullptr),
      apiclient_index_(nullptr),
      pending_requests_(std::make_shared<client::PendingRequests>()),
      init_in_progress_(false) {}

IndexLayerClientImpl::~IndexLayerClientImpl() {
  tokenList_.CancelAll();
  pending_requests_->CancelAllAndWait();
}

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

  ul.unlock();

  return ApiClientLookup::LookupApi(self->apiclient_blob_, "blob", "v1",
                                    self->catalog_, blobApi_callback);
}

void IndexLayerClientImpl::CancelPendingRequests() {
  pending_requests_->CancelAll();
  tokenList_.CancelAll();
}

client::CancellableFuture<PublishIndexResponse>
IndexLayerClientImpl::PublishIndex(const model::PublishIndexRequest& request) {
  auto promise = std::make_shared<std::promise<PublishIndexResponse> >();
  auto cancel_token =
      PublishIndex(request, [promise](PublishIndexResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<PublishIndexResponse>(cancel_token, promise);
}

client::CancellationToken IndexLayerClientImpl::PublishIndex(
    model::PublishIndexRequest request, const PublishIndexCallback& callback) {
  auto publish_task =
      [=](client::CancellationContext context) -> PublishIndexResponse {
    if (!request.GetData()) {
      return client::ApiError(client::ErrorCode::InvalidArgument,
                              "Request data empty.");
    }

    if (request.GetLayerId().empty()) {
      return client::ApiError(client::ErrorCode::InvalidArgument,
                              "Request layer Id empty.");
    }

    const auto data_handle = GenerateUuid();

    auto blob_api_response = ApiClientLookup::LookupApiClient(
        catalog_, context, "blob", "v1", settings_);
    if (!blob_api_response.IsSuccessful()) {
      return blob_api_response.GetError();
    }

    auto index_api_response = ApiClientLookup::LookupApiClient(
        catalog_, context, "index", "v1", settings_);
    if (!index_api_response.IsSuccessful()) {
      return index_api_response.GetError();
    }

    auto layer_settings_response = catalog_settings_.GetLayerSettings(
        context, request.GetBillingTag(), request.GetLayerId());
    if (!layer_settings_response.IsSuccessful()) {
      return layer_settings_response.GetError();
    }
    auto layer_settings = layer_settings_response.GetResult();
    if (layer_settings.content_type.empty()) {
      auto errmsg = boost::format(
                        "Unable to find the Layer ID (%1%) "
                        "provided in the PublishIndexRequest in the "
                        "Catalog specified when creating "
                        "this IndexLayerClient instance.") %
                    request.GetLayerId();
      return client::ApiError(client::ErrorCode::InvalidArgument, errmsg.str());
    }

    auto blob_response = BlobApi::PutBlob(
        blob_api_response.GetResult(), request.GetLayerId(),
        layer_settings.content_type, layer_settings.content_encoding,
        data_handle, request.GetData(), request.GetBillingTag(), context);

    if (!blob_response.IsSuccessful()) {
      return blob_response.GetError();
    }

    auto index = request.GetIndex();
    index.SetId(data_handle);
    auto insert_indexes_response = IndexApi::InsertIndexes(
        index_api_response.GetResult(), index, request.GetLayerId(),
        request.GetBillingTag(), context);

    if (!insert_indexes_response.IsSuccessful()) {
      return insert_indexes_response.GetError();
    }
    model::ResponseOkSingle res;
    res.SetTraceID(data_handle);
    return PublishIndexResponse(std::move(res));
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(publish_task), callback);
}

client::CancellableFuture<DeleteIndexDataResponse>
IndexLayerClientImpl::DeleteIndexData(
    const model::DeleteIndexDataRequest& request) {
  auto promise = std::make_shared<std::promise<DeleteIndexDataResponse> >();
  auto cancel_token =
      DeleteIndexData(request, [promise](DeleteIndexDataResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<DeleteIndexDataResponse>(cancel_token,
                                                            promise);
}

client::CancellationToken IndexLayerClientImpl::DeleteIndexData(
    const model::DeleteIndexDataRequest& request,
    const DeleteIndexDataCallback& callback) {
  if (request.GetLayerId().empty() || request.GetIndexId().empty()) {
    callback(DeleteIndexDataResponse(
        client::ApiError(client::ErrorCode::InvalidArgument,
                         "Request layer ID or Index Id is not defined.")));
    return client::CancellationToken();
  }

  auto layer_id = request.GetLayerId();
  auto index_id = request.GetIndexId();

  auto op_id = tokenList_.GetNextId();
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto self = shared_from_this();

  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(op_id);
    callback(DeleteIndexDataResponse(client::ApiError(
        client::ErrorCode::Cancelled, "Operation cancelled.", true)));
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
          callback(DeleteBlobRespone(client::ApiNoResult()));
        });
  };

  auto init_api_client_callback =
      [=](boost::optional<client::ApiError> init_api_error) {
        if (init_api_error) {
          self->tokenList_.RemoveTask(op_id);
          callback(DeleteBlobRespone(init_api_error.get()));
          return;
        }

        cancel_context->ExecuteOrCancelled(delete_index_data_function,
                                           cancel_function);
      };

  auto init_api_client_function = [=]() -> client::CancellationToken {
    return self->InitApiClients(cancel_context, init_api_client_callback);
  };

  cancel_context->ExecuteOrCancelled(init_api_client_function, cancel_function);

  auto ret = client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
  tokenList_.AddTask(op_id, ret);
  return ret;
}

client::CancellableFuture<UpdateIndexResponse>
IndexLayerClientImpl::UpdateIndex(const model::UpdateIndexRequest& request) {
  auto promise = std::make_shared<std::promise<UpdateIndexResponse> >();
  auto cancel_token =
      UpdateIndex(request, [promise](UpdateIndexResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<UpdateIndexResponse>(cancel_token, promise);
}

client::CancellationToken IndexLayerClientImpl::UpdateIndex(
    const model::UpdateIndexRequest& request,
    const UpdateIndexCallback& callback) {
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto self = shared_from_this();

  auto op_id = tokenList_.GetNextId();
  auto cancel_function = [=]() {
    self->tokenList_.RemoveTask(op_id);
    callback(UpdateIndexResponse(client::ApiError(
        client::ErrorCode::Cancelled, "Operation cancelled.", true)));
  };

  auto updateIndex_callback = [=](UpdateIndexResponse update_index_response) {
    self->tokenList_.RemoveTask(op_id);
    if (!update_index_response.IsSuccessful()) {
      callback(UpdateIndexResponse(update_index_response.GetError()));
      return;
    }
    callback(UpdateIndexResponse(client::ApiNoResult()));
  };

  auto UpdateIndex_function = [=]() -> client::CancellationToken {
    return IndexApi::performUpdate(*self->apiclient_index_, request,
                                   boost::none, updateIndex_callback);
  };

  cancel_context->ExecuteOrCancelled(
      [=]() -> client::CancellationToken {
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
