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

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/context/Context.h>
#include <olp/core/thread/TaskScheduler.h>
#include "PrefetchTilesProvider.h"
#include "TaskContext.h"
#include "repositories/ApiRepository.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsRepository.h"
#include "repositories/PrefetchTilesRepository.h"

namespace olp {
namespace dataservice {
namespace read {

VersionedLayerClientImpl::VersionedLayerClientImpl(
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

  auto api_repo = std::make_shared<repository::ApiRepository>(
      catalog_, settings_, settings_->cache);

  auto catalog_repo = std::make_shared<repository::CatalogRepository>(
      catalog_, api_repo, settings_->cache);

  partition_repo_ = std::make_shared<repository::PartitionsRepository>(
      catalog_, api_repo, catalog_repo, settings_->cache);

  auto data_repo = std::make_shared<repository::DataRepository>(
      catalog_, api_repo, catalog_repo, partition_repo_, settings_->cache);

  auto prefetch_repo = std::make_shared<repository::PrefetchTilesRepository>(
      catalog_, api_repo, partition_repo_->GetPartitionsCacheRepository(),
      settings_);

  prefetch_provider_ = std::make_shared<PrefetchTilesProvider>(
      catalog_, api_repo, catalog_repo, data_repo, prefetch_repo, settings_);
}

VersionedLayerClientImpl::~VersionedLayerClientImpl() {
  pending_requests_->CancelPendingRequests();
}

client::CancellationToken VersionedLayerClientImpl::GetPartitions(
    PartitionsRequest partitions_request, PartitionsResponseCallback callback) {
  partitions_request.WithLayerId(layer_id_);
  client::CancellationToken token;
  int64_t request_key = pending_requests_->GenerateRequestPlaceholder();
  auto pending_requests = pending_requests_;
  auto request_callback = [pending_requests, request_key,
                           callback](PartitionsResponse response) {
    if (pending_requests->Remove(request_key)) {
      callback(response);
    }
  };
  if (CacheWithUpdate == partitions_request.GetFetchOption()) {
    token = partition_repo_->GetPartitions(
        partitions_request.WithFetchOption(CacheOnly), request_callback);
    auto onlineKey = pending_requests_->GenerateRequestPlaceholder();
    pending_requests_->Insert(
        partition_repo_->GetPartitions(
            partitions_request.WithFetchOption(OnlineIfNotFound),
            [pending_requests, onlineKey](PartitionsResponse) {
              pending_requests->Remove(onlineKey);
            }),
        onlineKey);
  } else {
    token =
        partition_repo_->GetPartitions(partitions_request, request_callback);
  }
  pending_requests_->Insert(token, request_key);
  return token;
}

client::CancellationToken VersionedLayerClientImpl::GetData(
    DataRequest request, DataResponseCallback callback) {
  auto add_task = [&](DataRequest& request, DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = *settings_;
    auto pending_requests = pending_requests_;

    auto data_task = [=](client::CancellationContext context) {
      return repository::DataRepository::GetVersionedData(
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

client::CancellationToken VersionedLayerClientImpl::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback) {
  const int64_t request_key = pending_requests_->GenerateRequestPlaceholder();
  auto pending_requests = pending_requests_;
  auto request_callback = [=](PrefetchTilesResponse response) {
    pending_requests->Remove(request_key);
    if (callback) {
      callback(std::move(response));
    }
  };

  request.WithLayerId(layer_id_);
  auto token = prefetch_provider_->PrefetchTiles(request, request_callback);
  pending_requests->Insert(token, request_key);
  return token;
}

client::CancellableFuture<PrefetchTilesResponse>
VersionedLayerClientImpl::PrefetchTiles(PrefetchTilesRequest request) {
  auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
  auto cancel_token = PrefetchTiles(std::move(request),
                                    [promise](PrefetchTilesResponse response) {
                                      promise->set_value(std::move(response));
                                    });
  return client::CancellableFuture<PrefetchTilesResponse>(cancel_token,
                                                          promise);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
