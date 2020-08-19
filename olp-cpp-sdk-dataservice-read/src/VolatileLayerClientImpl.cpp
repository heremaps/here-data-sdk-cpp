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
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/PrefetchTileResult.h>

#include "Common.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataCacheRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsCacheRepository.h"
#include "repositories/PartitionsRepository.h"
#include "repositories/PrefetchTilesRepository.h"

#include "PrefetchJob.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "VolatileLayerClientImpl";
constexpr auto kQuadTreeDepth = 4;

bool IsOnlyInputTiles(const PrefetchTilesRequest& request) {
  return !(request.GetMinLevel() <= request.GetMaxLevel() &&
           request.GetMaxLevel() < geo::TileKey::LevelCount &&
           request.GetMinLevel() < geo::TileKey::LevelCount);
}
}  // namespace

VolatileLayerClientImpl::VolatileLayerClientImpl(
    client::HRN catalog, std::string layer_id,
    client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      settings_(std::move(settings)),
      pending_requests_(std::make_shared<client::PendingRequests>()),
      lookup_client_(catalog_, settings_) {
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
    auto lookup_client = lookup_client_;

    auto data_task = [=](client::CancellationContext context) {
      repository::PartitionsRepository repository(
          std::move(catalog), std::move(settings), std::move(lookup_client));
      return repository.GetVolatilePartitions(std::move(layer_id), request,
                                              std::move(context));
    };

    return AddTask(settings.task_scheduler, pending_requests_,
                   std::move(data_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_partitions), std::move(request),
                       std::move(callback));
}

client::CancellableFuture<PartitionsResponse>
VolatileLayerClientImpl::GetPartitions(PartitionsRequest request) {
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
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
    auto lookup_client = lookup_client_;

    auto partitions_task = [=](client::CancellationContext context) {
      repository::DataRepository repository(
          std::move(catalog), std::move(settings), std::move(lookup_client));
      return repository.GetVolatileData(layer_id, request, context);
    };

    return AddTask(settings.task_scheduler, pending_requests_,
                   std::move(partitions_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_data), std::move(request),
                       std::move(callback));
}

client::CancellableFuture<DataResponse> VolatileLayerClientImpl::GetData(
    DataRequest request) {
  auto promise = std::make_shared<std::promise<DataResponse>>();
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

bool VolatileLayerClientImpl::RemoveFromCache(const geo::TileKey& tile) {
  auto partition_id = tile.ToHereTile();
  return RemoveFromCache(partition_id);
}

client::CancellationToken VolatileLayerClientImpl::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback) {
  // Used as empty response to be able to execute initial task
  using EmptyResponse = Response<PrefetchTileNoError>;
  using client::CancellationContext;
  using client::ErrorCode;

  auto catalog = catalog_;
  auto layer_id = layer_id_;
  auto settings = settings_;
  auto pending_requests = pending_requests_;
  auto lookup_client = lookup_client_;

  auto token = AddTask(
      settings.task_scheduler, pending_requests,
      [=](CancellationContext context) mutable -> EmptyResponse {
        const auto& tile_keys = request.GetTileKeys();
        if (tile_keys.empty()) {
          OLP_SDK_LOG_WARNING_F(kLogTag,
                                "PrefetchTiles : invalid request, layer=%s",
                                layer_id.c_str());
          return {{ErrorCode::InvalidArgument, "Empty tile key list"}};
        }

        const auto key = request.CreateKey(layer_id);
        OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles: using key=%s", key.c_str());

        // Calculate the minimal set of Tile keys and depth to
        // cover tree.
        bool request_only_input_tiles = IsOnlyInputTiles(request);
        unsigned int min_level =
            (request_only_input_tiles
                 ? static_cast<unsigned int>(geo::TileKey::LevelCount)
                 : request.GetMinLevel());
        unsigned int max_level =
            (request_only_input_tiles
                 ? static_cast<unsigned int>(geo::TileKey::LevelCount)
                 : request.GetMaxLevel());

        repository::PrefetchTilesRepository repository(catalog, layer_id,
                                                       settings, lookup_client);
        auto sliced_tiles =
            repository.GetSlicedTiles(tile_keys, min_level, max_level);

        if (sliced_tiles.empty()) {
          OLP_SDK_LOG_WARNING_F(kLogTag,
                                "PrefetchTiles: tile/level mismatch, key=%s",
                                key.c_str());
          return {{ErrorCode::InvalidArgument, "TileKeys/levels mismatch"}};
        }

        OLP_SDK_LOG_DEBUG_F(kLogTag, "PrefetchTiles, subquads=%zu, key=%s",
                            sliced_tiles.size(), key.c_str());

        auto shared_settings =
            std::make_shared<client::OlpClientSettings>(settings);

        auto query = [=](geo::TileKey root,
                         client::CancellationContext inner_context) mutable {
          return repository.GetVolatileSubQuads(root, kQuadTreeDepth,
                                                inner_context);
        };

        auto filter = [=](QueryResult tiles) mutable {
          if (request_only_input_tiles) {
            return repository.FilterTilesByList(request, std::move(tiles));
          } else {
            return repository.FilterTilesByLevel(request, std::move(tiles));
          }
        };

        auto billing_tag = request.GetBillingTag();
        auto download = [=](std::string data_handle,
                            client::CancellationContext inner_context) mutable {
          if (data_handle.empty()) {
            return BlobApi::DataResponse(
                client::ApiError(client::ErrorCode::NotFound, "Not found"));
          }
          repository::DataCacheRepository data_cache_repository(
              catalog, shared_settings->cache);
          if (data_cache_repository.IsCached(layer_id, data_handle)) {
            return BlobApi::DataResponse(nullptr);
          }

          repository::DataRepository repository(catalog, *shared_settings,
                                                lookup_client);
          // Fetch from online
          return repository.GetVolatileData(layer_id,
                                            DataRequest()
                                                .WithDataHandle(data_handle)
                                                .WithBillingTag(billing_tag),
                                            inner_context);
        };

        std::vector<geo::TileKey> roots;
        roots.reserve(sliced_tiles.size());

        std::transform(
            sliced_tiles.begin(), sliced_tiles.end(), std::back_inserter(roots),
            [](const repository::RootTilesForRequest::value_type& root) {
              return root.first;
            });

        context.ExecuteOrCancelled([&]() {
          return PrefetchHelper::Prefetch(
              std::move(roots), std::move(query), std::move(filter),
              std::move(download), std::move(callback), nullptr,
              settings.task_scheduler, pending_requests);
        });

        return EmptyResponse(PrefetchTileNoError());
      },
      // Because the handling of prefetch tiles responses is performed by the
      // inner-task, no need to set a callback here. Otherwise, the user would
      // be notified with empty results.
      // It is possible to not invoke inner task, when it was cancelled before
      // execution.
      [callback](EmptyResponse response) {
        // Inner task only generates successfull result
        if (!response.IsSuccessful()) {
          callback(response.GetError());
        }
      });

  return token;
}

client::CancellableFuture<PrefetchTilesResponse>
VolatileLayerClientImpl::PrefetchTiles(PrefetchTilesRequest request) {
  auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
  auto callback = [=](PrefetchTilesResponse resp) {
    promise->set_value(std::move(resp));
  };
  auto token = PrefetchTiles(std::move(request), std::move(callback));
  return client::CancellableFuture<PrefetchTilesResponse>(token, promise);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
