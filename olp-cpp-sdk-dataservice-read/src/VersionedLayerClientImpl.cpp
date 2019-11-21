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
#include <olp/core/client/PendingRequests.h>
#include <olp/core/client/TaskContext.h>
#include <olp/core/context/Context.h>
#include <olp/core/thread/TaskScheduler.h>

#include "Common.h"
#include "PrefetchTilesProvider.h"
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
      pending_requests_(std::make_shared<client::PendingRequests>()) {
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
      catalog_, layer_id_, api_repo, catalog_repo, settings_->cache);

  auto data_repo = std::make_shared<repository::DataRepository>(
      catalog_, layer_id_, api_repo, catalog_repo, partition_repo_,
      settings_->cache);

  auto prefetch_repo = std::make_shared<repository::PrefetchTilesRepository>(
      catalog_, layer_id_, api_repo,
      partition_repo_->GetPartitionsCacheRepository(), settings_);

  prefetch_provider_ = std::make_shared<PrefetchTilesProvider>(
      catalog_, layer_id_, api_repo, catalog_repo, data_repo, prefetch_repo,
      settings_);
}

VersionedLayerClientImpl::~VersionedLayerClientImpl() {
  pending_requests_->CancelPendingRequests();
}

client::CancellationToken VersionedLayerClientImpl::GetPartitions(
    PartitionsRequest request, PartitionsResponseCallback callback) {
  auto schedule_get_partitions = [&](PartitionsRequest request,
                                     PartitionsResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = *settings_;

    auto partitions_task = [=](client::CancellationContext context) {
      return repository::PartitionsRepository::GetVersionedPartitions(
          catalog, layer_id, context, std::move(request), settings);
    };

    return AddTask(task_scheduler_, pending_requests_,
                   std::move(partitions_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_partitions), std::move(request),
                       std::move(callback));
}

client::CancellableFuture<PartitionsResponse>
VersionedLayerClientImpl::GetPartitions(PartitionsRequest partitions_request) {
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto cancel_token = GetPartitions(std::move(partitions_request),
                                    [promise](PartitionsResponse response) {
                                      promise->set_value(std::move(response));
                                    });
  return client::CancellableFuture<PartitionsResponse>(std::move(cancel_token),
                                                       std::move(promise));
}

client::CancellationToken VersionedLayerClientImpl::GetData(
    DataRequest request, DataResponseCallback callback) {
  auto schedule_get_data = [&](DataRequest request,
                               DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = *settings_;
    
    auto data_task = [=](client::CancellationContext context) {
      return repository::DataRepository::GetVersionedData(
          std::move(catalog), std::move(layer_id), std::move(request), context,
          std::move(settings));
    };

    return AddTask(task_scheduler_, pending_requests_, std::move(data_task),
                   std::move(callback));
  };
  return ScheduleFetch(std::move(schedule_get_data), std::move(request),
                       std::move(callback));
}

client::CancellableFuture<DataResponse> VersionedLayerClientImpl::GetData(
    DataRequest data_request) {
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto cancel_token =
      GetData(std::move(data_request), [promise](DataResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<DataResponse>(std::move(cancel_token),
                                                 std::move(promise));
}

client::CancellationToken VersionedLayerClientImpl::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback) {
  const int64_t request_key = pending_requests_->GenerateRequestPlaceholder();
  auto pending_requests = pending_requests_;
  auto request_callback = [=](PrefetchTilesResponse response) {
    if (pending_requests->Remove(request_key) && callback) {
      callback(std::move(response));
    }
  };

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
