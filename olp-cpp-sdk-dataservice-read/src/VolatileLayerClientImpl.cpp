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

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

#include "TaskContext.h"
#include "repositories/ApiRepository.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsRepository.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "VolatileLayerClientImpl";
}  // namespace

VolatileLayerClientImpl::VolatileLayerClientImpl(
    client::HRN catalog, std::string layer_id,
    client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      settings_(
          std::make_shared<client::OlpClientSettings>(std::move(settings))),
      pending_requests_(std::make_shared<PendingRequests>()) {
  if (!settings_->cache) {
    settings_->cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
  // to avoid capturing task scheduler inside a task, we need a copy of settings
  // without the scheduler
  task_scheduler_ = std::move(settings_->task_scheduler);

  auto cache = settings_->cache;

  auto api_repo =
      std::make_shared<repository::ApiRepository>(catalog_, settings_, cache);

  auto catalog_repo = std::make_shared<repository::CatalogRepository>(
      catalog_, api_repo, cache);

  partition_repo_ = std::make_shared<repository::PartitionsRepository>(
      catalog_, api_repo, catalog_repo, cache);
}

VolatileLayerClientImpl::~VolatileLayerClientImpl() {
  pending_requests_->CancelPendingRequests();
}

client::CancellationToken VolatileLayerClientImpl::GetPartitions(
    PartitionsRequest request, PartitionsResponseCallback callback) {
  request.WithLayerId(layer_id_);
  client::CancellationToken token;
  int64_t request_key = pending_requests_->GenerateRequestPlaceholder();
  auto pending_requests = pending_requests_;
  auto request_callback = [pending_requests, request_key,
                           callback](PartitionsResponse response) {
    if (pending_requests->Remove(request_key)) {
      callback(response);
    }
  };
  if (CacheWithUpdate == request.GetFetchOption()) {
    token = partition_repo_->GetPartitions(request.WithFetchOption(CacheOnly),
                                           request_callback);
    auto onlineKey = pending_requests_->GenerateRequestPlaceholder();
    pending_requests_->Insert(
        partition_repo_->GetPartitions(
            request.WithFetchOption(OnlineIfNotFound),
            [pending_requests, onlineKey](PartitionsResponse) {
              pending_requests->Remove(onlineKey);
            }),
        onlineKey);
  } else {
    token = partition_repo_->GetPartitions(request, request_callback);
  }
  pending_requests_->Insert(token, request_key);
  return token;
}

client::CancellableFuture<PartitionsResponse>
VolatileLayerClientImpl::GetPartitions(PartitionsRequest request) {
  auto promise = std::make_shared<std::promise<PartitionsResponse> >();
  auto callback = [=](PartitionsResponse resp) {
    promise->set_value(std::move(resp));
  };
  auto token = GetPartitions(std::move(request), std::move(callback));
  return olp::client::CancellableFuture<PartitionsResponse>(token, promise);
}

client::CancellationToken VolatileLayerClientImpl::GetData(
    DataRequest request, DataResponseCallback callback) {
  auto add_task = [&](DataRequest& request, DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = *settings_;
    auto pending_requests = pending_requests_;

    auto data_task = [=](client::CancellationContext context) {
      return repository::DataRepository::GetVolatileData(
          catalog, layer_id, request, context, settings);
    };

    auto context =
        TaskContext::Create(std::move(data_task), std::move(callback));

    pending_requests->Insert(context);

    repository::ExecuteOrSchedule(task_scheduler_, [=]() {
      context.Execute();
      pending_requests->Remove(context);
    });

    return context.CancelToken();
  };

  if (request.GetFetchOption() == FetchOptions::CacheWithUpdate) {
    auto cache_token = add_task(
        request.WithFetchOption(FetchOptions::CacheOnly), std::move(callback));
    auto online_token =
        add_task(request.WithFetchOption(FetchOptions::OnlineOnly), nullptr);

    return client::CancellationToken([=]() {
      cache_token.cancel();
      online_token.cancel();
    });
  } else {
    return add_task(request, std::move(callback));
  }
}

client::CancellableFuture<DataResponse> VolatileLayerClientImpl::GetData(
    DataRequest request) {
  auto promise = std::make_shared<std::promise<DataResponse> >();
  auto callback = [=](DataResponse resp) {
    promise->set_value(std::move(resp));
  };
  auto token = GetData(std::move(request), std::move(callback));
  return olp::client::CancellableFuture<DataResponse>(token, promise);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
