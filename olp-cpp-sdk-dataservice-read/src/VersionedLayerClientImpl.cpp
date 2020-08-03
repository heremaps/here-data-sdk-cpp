/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/client/TaskContext.h>
#include <olp/core/context/Context.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include "Common.h"
#include "ExtendedApiResponseHelpers.h"
#include "PrefetchJob.h"
#include "ReleaseDependencyResolver.h"
#include "generated/api/QueryApi.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataCacheRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/PartitionsRepository.h"
#include "repositories/PrefetchTilesRepository.h"

// Needed to avoid endless warnings from GetVersion/WithVersion
#include <olp/core/porting/warning_disable.h>
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "VersionedLayerClientImpl";
constexpr int64_t kInvalidVersion = -1;
constexpr auto kQuadTreeDepth = 4;
}  // namespace

VersionedLayerClientImpl::VersionedLayerClientImpl(
    client::HRN catalog, std::string layer_id,
    boost::optional<int64_t> catalog_version,
    client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      settings_(std::move(settings)),
      pending_requests_(std::make_shared<client::PendingRequests>()),
      catalog_version_(catalog_version ? catalog_version.get()
                                       : kInvalidVersion),
      lookup_client_(catalog_, settings_) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
}

VersionedLayerClientImpl::~VersionedLayerClientImpl() {
  pending_requests_->CancelAllAndWait();
}

bool VersionedLayerClientImpl::CancelPendingRequests() {
  OLP_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  return pending_requests_->CancelAll();
}

client::CancellationToken VersionedLayerClientImpl::GetPartitions(
    PartitionsRequest request, PartitionsResponseCallback callback) {
  if (request.GetFetchOption() == CacheWithUpdate) {
    auto task = [](client::CancellationContext) -> PartitionsResponse {
      return {{client::ErrorCode::InvalidArgument,
               "CacheWithUpdate option can not be used for versioned "
               "layer"}};
    };
    return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                   std::move(callback));
  }

  auto schedule_get_partitions = [&](PartitionsRequest request,
                                     PartitionsResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;
    auto lookup_client = lookup_client_;

    auto partitions_task =
        [=](client::CancellationContext context) mutable -> PartitionsResponse {
      auto version_response = GetVersion(request.GetBillingTag(),
                                         request.GetFetchOption(), context);
      if (!version_response.IsSuccessful()) {
        return version_response.GetError();
      }
      const auto version = version_response.GetResult().GetVersion();

      repository::PartitionsRepository repository(
          std::move(catalog), std::move(settings), std::move(lookup_client));
      return repository.GetVersionedPartitions(layer_id, request, version,
                                               context);
    };

    return AddTask(settings.task_scheduler, pending_requests_,
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
  if (request.GetFetchOption() == CacheWithUpdate) {
    auto task = [](client::CancellationContext) -> DataResponse {
      return {{client::ErrorCode::InvalidArgument,
               "CacheWithUpdate option can not be used for versioned "
               "layer"}};
    };
    return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                   std::move(callback));
  }

  auto schedule_get_data = [&](DataRequest request,
                               DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;
    auto lookup_client = lookup_client_;

    auto data_task =
        [=](client::CancellationContext context) mutable -> DataResponse {
      int64_t version = -1;
      if (!request.GetDataHandle()) {
        auto version_response = GetVersion(request.GetBillingTag(),
                                           request.GetFetchOption(), context);
        if (!version_response.IsSuccessful()) {
          return version_response.GetError();
        }
        version = version_response.GetResult().GetVersion();
      }

      repository::DataRepository repository(
          std::move(catalog), std::move(settings), std::move(lookup_client));
      return repository.GetVersionedData(layer_id, request, version, context);
    };

    return AddTask(settings.task_scheduler, pending_requests_,
                   std::move(data_task), std::move(callback));
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
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback,
    PrefetchStatusCallback status_callback) {
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
        if (request.GetTileKeys().empty()) {
          OLP_SDK_LOG_WARNING_F(kLogTag,
                                "PrefetchTiles : invalid request, layer=%s",
                                layer_id.c_str());
          return {{ErrorCode::InvalidArgument, "Empty tile key list"}};
        }

        auto response =
            GetVersion(request.GetBillingTag(), OnlineIfNotFound, context);

        if (!response.IsSuccessful()) {
          OLP_SDK_LOG_WARNING_F(
              kLogTag, "PrefetchTiles: getting catalog version failed, key=%s",
              request.CreateKey(layer_id).c_str());
          return response.GetError();
        }
        auto version = response.GetResult().GetVersion();

        const auto key = request.CreateKey(layer_id);
        OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles: using key=%s", key.c_str());

        // Calculate the minimal set of Tile keys and depth to
        // cover tree.
        bool request_only_input_tiles =
            !(request.GetMinLevel() <= request.GetMaxLevel() &&
              request.GetMaxLevel() < geo::TileKey::LevelCount &&
              request.GetMinLevel() < geo::TileKey::LevelCount);
        unsigned int min_level =
            (request_only_input_tiles
                 ? static_cast<unsigned int>(geo::TileKey::LevelCount)
                 : request.GetMinLevel());
        unsigned int max_level =
            (request_only_input_tiles
                 ? static_cast<unsigned int>(geo::TileKey::LevelCount)
                 : request.GetMaxLevel());

        repository::PrefetchTilesRepository repository(catalog, settings,
                                                       lookup_client);
        auto sliced_tiles = repository.GetSlicedTiles(request.GetTileKeys(),
                                                      min_level, max_level);

        if (sliced_tiles.empty()) {
          OLP_SDK_LOG_WARNING_F(kLogTag,
                                "PrefetchTiles: tile/level mismatch, key=%s",
                                key.c_str());
          return {{ErrorCode::InvalidArgument, "TileKeys/levels mismatch"}};
        }

        OLP_SDK_LOG_DEBUG_F(kLogTag, "PrefetchTiles, subquads=%zu, key=%s",
                            sliced_tiles.size(), key.c_str());

        auto sub_tiles = repository.GetSubTiles(layer_id, request, version,
                                                sliced_tiles, context);
        if (!sub_tiles.IsSuccessful()) {
          return sub_tiles.GetError();
        }

        auto tiles_result = repository.FilterSkippedTiles(
            request, request_only_input_tiles, sub_tiles.MoveResult());

        if (tiles_result.empty()) {
          return EmptyResponse(
              {olp::client::ErrorCode::NotFound, "No tiles to prefetch"});
        }

        OLP_SDK_LOG_INFO_F(kLogTag, "Prefetch start, key=%s, tiles=%zu",
                           key.c_str(), tiles_result.size());

        // Settings structure consumes a 536 bytes of heap memory when captured
        // in lambda, shared pointer (16 bytes) saves 520 bytes of heap memory.
        // When users prefetch few hundreds tiles it could save few mb.
        auto shared_settings =
            std::make_shared<client::OlpClientSettings>(settings);

        auto prefetch_job = std::make_shared<PrefetchJob>(
            std::move(callback), std::move(status_callback),
            tiles_result.size());

        std::for_each(
            tiles_result.begin(), tiles_result.end(),
            [&](const repository::SubQuadsResult::value_type& sub_quad) {
              const auto& tile = sub_quad.first;
              const auto& handle = sub_quad.second;
              const auto& biling_tag = request.GetBillingTag();

              AddTask(settings.task_scheduler, pending_requests,
                      [=](CancellationContext inner_context) -> EmptyResponse {
                        if (handle.empty()) {
                          prefetch_job->CompleteTask(
                              tile, {client::ErrorCode::NotFound, "Not found"});
                          return PrefetchTileNoError{};
                        }
                        repository::DataCacheRepository data_cache_repository(
                            catalog, shared_settings->cache);
                        if (data_cache_repository.IsCached(layer_id, handle)) {
                          prefetch_job->CompleteTask(tile);
                          return PrefetchTileNoError{};
                        }

                        // Fetch from online
                        repository::DataRepository repository(
                            catalog, *shared_settings, lookup_client);
                        auto result = repository.GetVersionedData(
                            layer_id,
                            DataRequest().WithDataHandle(handle).WithBillingTag(
                                biling_tag),
                            version, inner_context);

                        if (result.IsSuccessful()) {
                          prefetch_job->CompleteTask(
                              tile, GetNetworkStatistics(result));
                        } else {
                          prefetch_job->CompleteTask(
                              tile, result.GetError(),
                              GetNetworkStatistics(result));
                        }
                        return PrefetchTileNoError{};
                      },
                      [=](EmptyResponse) {}, prefetch_job->AddTask());
            });

        context.ExecuteOrCancelled([&]() {
          return client::CancellationToken(
              [=]() { prefetch_job->CancelOperation(); });
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
          if (response.GetError().GetErrorCode() ==
              olp::client::ErrorCode::NotFound) {
            callback(PrefetchTilesResult());
          } else {
            callback(response.GetError());
          }
        }
      });

  return token;
}

client::CancellableFuture<PrefetchTilesResponse>
VersionedLayerClientImpl::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchStatusCallback status_callback) {
  auto promise = std::make_shared<std::promise<PrefetchTilesResponse>>();
  auto cancel_token = PrefetchTiles(std::move(request),
                                    [promise](PrefetchTilesResponse response) {
                                      promise->set_value(std::move(response));
                                    },
                                    std::move(status_callback));
  return client::CancellableFuture<PrefetchTilesResponse>(cancel_token,
                                                          promise);
}

CatalogVersionResponse VersionedLayerClientImpl::GetVersion(
    boost::optional<std::string> billing_tag, const FetchOptions& fetch_options,
    const client::CancellationContext& context) {
  auto version = catalog_version_.load();
  if (version != kInvalidVersion) {
    model::VersionResponse response;
    response.SetVersion(version);
    return response;
  }

  CatalogVersionRequest request;
  request.WithBillingTag(billing_tag);
  request.WithFetchOption(fetch_options);

  repository::CatalogRepository repository(catalog_, settings_, lookup_client_);
  auto response = repository.GetLatestVersion(request, context);

  if (!response.IsSuccessful()) {
    return response;
  }

  if (!catalog_version_.compare_exchange_weak(
          version, response.GetResult().GetVersion())) {
    model::VersionResponse version_response;
    version_response.SetVersion(version);
    return version_response;
  }

  return response;
}

client::CancellationToken VersionedLayerClientImpl::GetData(
    TileRequest request, DataResponseCallback callback) {
  if (request.GetFetchOption() == CacheWithUpdate) {
    auto task = [](client::CancellationContext) -> DataResponse {
      return {{client::ErrorCode::InvalidArgument,
               "CacheWithUpdate option can not be used for versioned "
               "layer"}};
    };
    return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                   std::move(callback));
  }

  if (!request.GetTileKey().IsValid()) {
    auto task = [](client::CancellationContext) -> DataResponse {
      return {{client::ErrorCode::InvalidArgument, "Tile key is invalid"}};
    };
    return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                   std::move(callback));
  }

  auto schedule_get_data = [&](TileRequest request,
                               DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;
    auto pending_requests = pending_requests_;
    auto lookup_client = lookup_client_;

    auto data_task = [=](client::CancellationContext context) -> DataResponse {
      auto version_response = GetVersion(request.GetBillingTag(),
                                         request.GetFetchOption(), context);
      if (!version_response.IsSuccessful()) {
        return version_response.GetError();
      }

      repository::DataRepository repository(
          std::move(catalog), std::move(settings), std::move(lookup_client));
      return repository.GetVersionedTile(
          layer_id, request, version_response.GetResult().GetVersion(),
          context);
    };

    return AddTask(settings.task_scheduler, pending_requests,
                   std::move(data_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_data), std::move(request),
                       std::move(callback));
}

client::CancellableFuture<DataResponse> VersionedLayerClientImpl::GetData(
    TileRequest request) {
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto cancel_token =
      GetData(std::move(request), [promise](DataResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<DataResponse>(std::move(cancel_token),
                                                 std::move(promise));
}

bool VersionedLayerClientImpl::RemoveFromCache(
    const std::string& partition_id) {
  repository::PartitionsCacheRepository partitions_cache_repository(
      catalog_, settings_.cache);
  boost::optional<model::Partition> partition;
  if (!partitions_cache_repository.ClearPartitionMetadata(
          catalog_version_.load(), partition_id, layer_id_, partition)) {
    return false;
  }

  if (!partition) {
    return true;
  }

  repository::DataCacheRepository data_cache_repository(catalog_,
                                                        settings_.cache);
  return data_cache_repository.Clear(layer_id_,
                                     partition.get().GetDataHandle());
}

bool VersionedLayerClientImpl::RemoveFromCache(const geo::TileKey& tile) {
  read::QuadTreeIndex cached_tree;
  repository::PartitionsCacheRepository partitions_cache_repository(
      catalog_, settings_.cache);
  if (partitions_cache_repository.FindQuadTree(
          layer_id_, catalog_version_.load(), tile, cached_tree)) {
    auto data = cached_tree.Find(tile, false);
    if (!data) {
      return true;
    }
    repository::DataCacheRepository data_cache_repository(catalog_,
                                                          settings_.cache);
    auto result = data_cache_repository.Clear(layer_id_, data->data_handle);
    if (result) {
      auto index_data = cached_tree.GetIndexData();
      for (const auto& ind : index_data) {
        if (ind.tile_key != tile &&
            data_cache_repository.IsCached(layer_id_, ind.data_handle)) {
          return true;
        }
      }
      return partitions_cache_repository.ClearQuadTree(
          layer_id_, cached_tree.GetRootTile(), kQuadTreeDepth,
          catalog_version_.load());
    }
    return result;
  }
  return true;
}

bool VersionedLayerClientImpl::IsCached(const std::string& partition_id) const {
  repository::PartitionsCacheRepository partitions_cache_repository(
      catalog_, settings_.cache);
  std::string handle;
  if (partitions_cache_repository.GetPartitionHandle(
          catalog_version_.load(), partition_id, layer_id_, handle)) {
    repository::DataCacheRepository data_cache_repository(catalog_,
                                                          settings_.cache);
    return data_cache_repository.IsCached(layer_id_, handle);
  }
  return false;
}

bool VersionedLayerClientImpl::IsCached(const geo::TileKey& tile) const {
  read::QuadTreeIndex cached_tree;

  repository::PartitionsCacheRepository partitions_cache_repository(
      catalog_, settings_.cache);
  if (partitions_cache_repository.FindQuadTree(
          layer_id_, catalog_version_.load(), tile, cached_tree)) {
    auto data = cached_tree.Find(tile, false);
    if (!data) {
      return false;
    }
    repository::DataCacheRepository data_cache_repository(catalog_,
                                                          settings_.cache);
    return data_cache_repository.IsCached(layer_id_, data->data_handle);
  }
  return false;
}

client::CancellationToken VersionedLayerClientImpl::GetAggregatedData(
    TileRequest request, AggregatedDataResponseCallback callback) {
  auto catalog = catalog_;
  auto layer_id = layer_id_;
  auto settings = settings_;
  auto pending_requests = pending_requests_;
  auto lookup_client = lookup_client_;

  auto data_task =
      [=](client::CancellationContext context) -> AggregatedDataResponse {
    if (request.GetFetchOption() == CacheWithUpdate) {
      return {{client::ErrorCode::InvalidArgument,
               "CacheWithUpdate option can not be used for versioned "
               "layer"}};
    }

    if (!request.GetTileKey().IsValid()) {
      return {{client::ErrorCode::InvalidArgument, "Tile key is invalid"}};
    }

    auto version_response =
        GetVersion(request.GetBillingTag(), request.GetFetchOption(), context);
    if (!version_response.IsSuccessful()) {
      return version_response.GetError();
    }

    auto version = version_response.GetResult().GetVersion();
    repository::PartitionsRepository repository(catalog_, settings_,
                                                lookup_client_);
    auto partition_response =
        repository.GetAggregatedTile(layer_id, request, version, context);
    if (!partition_response.IsSuccessful()) {
      return partition_response.GetError();
    }

    const auto& fetch_partition = partition_response.GetResult();
    const auto fetch_tile_key =
        geo::TileKey::FromHereTile(fetch_partition.GetPartition());

    auto data_request = DataRequest()
                            .WithDataHandle(fetch_partition.GetDataHandle())
                            .WithFetchOption(request.GetFetchOption());

    repository::DataRepository data_repository(
        std::move(catalog), std::move(settings), std::move(lookup_client));
    auto data_response = data_repository.GetVersionedData(
        layer_id, data_request, version, context);

    if (!data_response.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetAggregatedData: failed to load data, key=%s, data_hanndle=%s",
          fetch_tile_key.ToHereTile().c_str(),
          fetch_partition.GetDataHandle().c_str());
      return data_response.GetError();
    }

    AggregatedDataResult result;
    result.SetTile(fetch_tile_key);
    result.SetData(data_response.MoveResult());

    return std::move(result);
  };

  return AddTask(settings.task_scheduler, pending_requests,
                 std::move(data_task), std::move(callback));
}

client::CancellableFuture<AggregatedDataResponse>
VersionedLayerClientImpl::GetAggregatedData(TileRequest request) {
  auto promise = std::make_shared<std::promise<AggregatedDataResponse>>();
  auto cancel_token = GetAggregatedData(
      std::move(request), [promise](AggregatedDataResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<AggregatedDataResponse>(
      std::move(cancel_token), std::move(promise));
}

bool VersionedLayerClientImpl::Protect(const TileKeys& tiles) {
  if (!settings_.cache) {
    return false;
  }
  auto version = catalog_version_.load();
  // could protect keys, which is already in cache
  // because we don't know quad tree which will contain tiles
  repository::DataCacheRepository data_cache_repository(catalog_,
                                                        settings_.cache);
  repository::PartitionsCacheRepository partitions_cache_repository(
      catalog_, settings_.cache);

  cache::KeyValueCache::KeyListType keys_to_protect;
  keys_to_protect.reserve(tiles.size() * 2);
  auto add_data_handle = [&](const geo::TileKey& tile,
                             const read::QuadTreeIndex& quad_tree) {
    auto data = quad_tree.Find(tile, false);
    if (data) {
      keys_to_protect.emplace_back(
          data_cache_repository.CreateKey(layer_id_, data->data_handle));
      return true;
    }
    return false;
  };

  std::vector<read::QuadTreeIndex> quad_trees;
  quad_trees.reserve(tiles.size());
  for (const auto& tile : tiles) {
    auto it =
        std::find_if(quad_trees.begin(), quad_trees.end(),
                     [&tile](const read::QuadTreeIndex& quad) {
                       auto root = quad.GetRootTile();
                       return (tile.Level() >= root.Level() &&
                               tile.Level() - root.Level() <= kQuadTreeDepth &&
                               root.IsParentOf(tile));
                     });

    if (it != quad_trees.end()) {
      add_data_handle(tile, *it);
    } else {
      read::QuadTreeIndex cached_tree;
      if (partitions_cache_repository.FindQuadTree(layer_id_, version, tile,
                                                   cached_tree) &&
          add_data_handle(tile, cached_tree)) {
        keys_to_protect.emplace_back(partitions_cache_repository.CreateQuadKey(
            layer_id_, cached_tree.GetRootTile(), kQuadTreeDepth, version));
        quad_trees.emplace_back(std::move(cached_tree));
      }
    }
  }

  if (keys_to_protect.empty()) {
    return false;
  }
  return settings_.cache->Protect(keys_to_protect);
}

bool VersionedLayerClientImpl::Release(const TileKeys& tiles) {
  if (!settings_.cache) {
    return false;
  }
  auto version = catalog_version_.load();

  auto tiles_dependency_resolver =
      ReleaseDependencyResolver(catalog_, layer_id_, version, settings_);
  const auto& keys_to_release =
      tiles_dependency_resolver.GetKeysToRelease(tiles);

  if (keys_to_release.empty()) {
    return false;
  }

  return settings_.cache->Release(keys_to_release);
}

PORTING_POP_WARNINGS()
}  // namespace read
}  // namespace dataservice
}  // namespace olp
