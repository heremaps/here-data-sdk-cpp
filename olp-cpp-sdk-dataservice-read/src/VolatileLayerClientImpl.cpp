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
#include <olp/core/client/PendingRequests.h>
#include <olp/core/client/TaskContext.h>
#include <olp/core/logging/Log.h>

#include "Common.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataCacheRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsCacheRepository.h"
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
      settings_(std::move(settings)),
      pending_requests_(std::make_shared<client::PendingRequests>()) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
}

VolatileLayerClientImpl::~VolatileLayerClientImpl() {
  pending_requests_->CancelAllAndWait();
}

bool VolatileLayerClientImpl::CancelPendingRequests() {
  OLP_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  return pending_requests_->CancelAll();
}

client::CancellationToken VolatileLayerClientImpl::GetPartitions(
    PartitionsRequest request, PartitionsResponseCallback callback) {
  auto schedule_get_partitions = [&](PartitionsRequest request,
                                     PartitionsResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;

    auto data_task = [=](client::CancellationContext context) {
      return repository::PartitionsRepository::GetVolatilePartitions(
          std::move(catalog), std::move(layer_id), std::move(context),
          std::move(request), std::move(settings));
    };

    return AddTask(settings.task_scheduler, pending_requests_,
                   std::move(data_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_partitions), std::move(request),
                       std::move(callback));
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
  auto schedule_get_data = [&](DataRequest request,
                               DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;

    auto partitions_task = [=](client::CancellationContext context) {
      return repository::DataRepository::GetVolatileData(
          catalog, layer_id, request, context, settings);
    };

    return AddTask(settings.task_scheduler, pending_requests_,
                   std::move(partitions_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_data), std::move(request),
                       std::move(callback));
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

bool VolatileLayerClientImpl::RemoveFromCache(const std::string& partition_id) {
  repository::PartitionsCacheRepository cache_repository(catalog_,
                                                         settings_.cache);
  boost::optional<model::Partition> partition;
  if (!cache_repository.ClearPartitionMetadata(boost::none, partition_id,
                                               layer_id_, partition)) {
    return false;
  }

  if (!partition) {
    // partition are not stored in cache
    return true;
  }

  repository::DataCacheRepository data_repository(catalog_, settings_.cache);
  return data_repository.Clear(layer_id_, partition.get().GetDataHandle());
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
