/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/client/TaskContext.h>
#include <olp/core/context/Context.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include "Common.h"
#include "ExtendedApiResponseHelpers.h"
#include "PrefetchPartitionsHelper.h"
#include "PrefetchTilesHelper.h"
#include "ProtectDependencyResolver.h"
#include "ReleaseDependencyResolver.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataCacheRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/PartitionsRepository.h"
#include "repositories/PrefetchTilesRepository.h"

#include "repositories/AsyncJsonStream.h"
#include "repositories/PartitionsSaxHandler.h"

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
    porting::optional<int64_t> catalog_version,
    client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      settings_(std::move(settings)),
      catalog_version_(catalog_version ? *catalog_version : kInvalidVersion),
      lookup_client_(catalog_, settings_),
      task_sink_(settings_.task_scheduler) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
}

bool VersionedLayerClientImpl::CancelPendingRequests() {
  OLP_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  task_sink_.CancelTasks();
  return true;
}

client::CancellationToken VersionedLayerClientImpl::GetPartitions(
    PartitionsRequest request, PartitionsResponseCallback callback) {
  auto partitions_task =
      [this](PartitionsRequest partitions_request,
             client::CancellationContext context) -> PartitionsResponse {
    const auto fetch_option = partitions_request.GetFetchOption();
    if (fetch_option == CacheWithUpdate) {
      return client::ApiError::InvalidArgument(
          "CacheWithUpdate option can not be used for versioned layer");
    }

    auto version_response =
        GetVersion(partitions_request.GetBillingTag(), fetch_option, context);
    if (!version_response.IsSuccessful()) {
      return version_response.GetError();
    }

    const auto version = version_response.GetResult().GetVersion();

    repository::PartitionsRepository repository(catalog_, layer_id_, settings_,
                                                lookup_client_, mutex_storage_);
    return repository.GetVersionedPartitionsExtendedResponse(
        std::move(partitions_request), version, context);
  };

  return task_sink_.AddTask(
      std::bind(partitions_task, std::move(request), std::placeholders::_1),
      std::move(callback), thread::NORMAL);
}

client::CancellationToken VersionedLayerClientImpl::StreamLayerPartitions(
    PartitionsRequest request,
    PartitionsStreamCallback partition_stream_callback,
    CallbackNoResult callback) {
  auto async_stream = std::make_shared<repository::AsyncJsonStream>();

  auto request_task =
      [=](const client::CancellationContext& context) -> client::ApiNoResponse {
    auto version_response =
        GetVersion(olp::porting::none, FetchOptions::OnlineIfNotFound, context);
    if (!version_response.IsSuccessful()) {
      async_stream->CloseStream(version_response.GetError());
      return version_response.GetError();
    }

    const auto version = version_response.GetResult().GetVersion();

    repository::PartitionsRepository repository(catalog_, layer_id_, settings_,
                                                lookup_client_, mutex_storage_);

    repository.StreamPartitions(async_stream, version,
                                request.GetAdditionalFields(),
                                request.GetBillingTag(), context);

    return client::ApiNoResult{};
  };

  auto request_task_token = task_sink_.AddTask(
      std::bind(request_task, std::placeholders::_1), nullptr, thread::NORMAL);

  auto parse_task = [=](client::CancellationContext context) {
    repository::PartitionsRepository repository(catalog_, layer_id_, settings_,
                                                lookup_client_, mutex_storage_);

    return repository.ParsePartitionsStream(
        async_stream, partition_stream_callback, std::move(context));
  };

  auto parse_task_token =
      task_sink_.AddTask(std::bind(parse_task, std::placeholders::_1),
                         std::move(callback), thread::NORMAL);

  return client::CancellationToken([request_task_token, parse_task_token]() {
    request_task_token.Cancel();
    parse_task_token.Cancel();
  });
}

client::CancellableFuture<PartitionsResponse>
VersionedLayerClientImpl::GetPartitions(PartitionsRequest partitions_request) {
  auto promise = std::make_shared<std::promise<PartitionsResponse>>();
  auto cancel_token = GetPartitions(std::move(partitions_request),
                                    [promise](PartitionsResponse response) {
                                      promise->set_value(std::move(response));
                                    });
  return {cancel_token, std::move(promise)};
}

client::CancellationToken VersionedLayerClientImpl::GetData(
    DataRequest request, DataResponseCallback callback) {
  auto data_task =
      [=](const client::CancellationContext& context) mutable -> DataResponse {
    if (request.GetFetchOption() == CacheWithUpdate) {
      return client::ApiError::InvalidArgument(
          "CacheWithUpdate option can not be used for versioned layer");
    }

    int64_t version = -1;
    if (!request.GetDataHandle()) {
      auto version_response = GetVersion(request.GetBillingTag(),
                                         request.GetFetchOption(), context);
      if (!version_response.IsSuccessful()) {
        return version_response.GetError();
      }
      version = version_response.GetResult().GetVersion();
    }

    repository::DataRepository repository(catalog_, settings_, lookup_client_,
                                          mutex_storage_);
    return repository.GetVersionedData(layer_id_, request, version, context,
                                       settings_.propagate_all_cache_errors);
  };

  return task_sink_.AddTask(std::move(data_task), std::move(callback),
                            request.GetPriority());
}

client::CancellationToken VersionedLayerClientImpl::QuadTreeIndex(
    TileRequest tile_request, PartitionsResponseCallback callback) {
  auto data_task = [=](const client::CancellationContext& context) mutable
      -> PartitionsResponse {
    if (!tile_request.GetTileKey().IsValid()) {
      return client::ApiError::InvalidArgument("Tile key is invalid");
    }

    const auto& fetch_option = tile_request.GetFetchOption();
    if (fetch_option == CacheWithUpdate) {
      return client::ApiError::InvalidArgument(
          "CacheWithUpdate option can not be used for versioned layer");
    }

    auto version_response =
        GetVersion(tile_request.GetBillingTag(), fetch_option, context);
    if (!version_response) {
      return version_response.GetError();
    }

    const auto version = version_response.GetResult().GetVersion();

    repository::PartitionsRepository repository(catalog_, layer_id_, settings_,
                                                lookup_client_, mutex_storage_);

    static const std::vector<std::string> additional_fields = {
        PartitionsRequest::kChecksum, PartitionsRequest::kCrc,
        PartitionsRequest::kDataSize};

    auto partition_response =
        repository.GetTile(tile_request, version, context, additional_fields);
    if (!partition_response) {
      return {partition_response.GetError(), partition_response.GetPayload()};
    }

    model::Partitions result;
    result.GetMutablePartitions().emplace_back(partition_response.MoveResult());
    return {std::move(result), partition_response.GetPayload()};
  };

  return task_sink_.AddTask(std::move(data_task), std::move(callback),
                            tile_request.GetPriority());
}

client::CancellableFuture<DataResponse> VersionedLayerClientImpl::GetData(
    DataRequest data_request) {
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto cancel_token =
      GetData(std::move(data_request), [promise](DataResponse response) {
        promise->set_value(std::move(response));
      });
  return {cancel_token, std::move(promise)};
}

client::CancellationToken VersionedLayerClientImpl::PrefetchPartitions(
    PrefetchPartitionsRequest request,
    PrefetchPartitionsResponseCallback callback,
    PrefetchPartitionsStatusCallback status_callback) {
  using client::ApiError;
  using client::ErrorCode;

  client::CancellationContext execution_context;

  auto task = [=](client::CancellationContext context,
                  PrefetchPartitionsRequest& request) mutable {
    if (context.IsCancelled()) {
      callback(ApiError::Cancelled());
      return;
    }

    const auto key = request.CreateKey(layer_id_);

    if (!settings_.cache) {
      OLP_SDK_LOG_ERROR_F(
          kLogTag,
          "PrefetchPartitions: cache is missing, aborting, hrn=%s, key=%s",
          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      callback(
          ApiError::PreconditionFailed("Unable to prefetch without a cache"));
      return;
    }

    if (request.GetPartitionIds().empty()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "PrefetchPartitions: invalid request, catalog=%s, key=%s",
          catalog_.ToCatalogHRNString().c_str(), key.c_str());
      callback(ApiError::InvalidArgument("Empty partitions list"));
      return;
    }

    const auto& billing_tag = request.GetBillingTag();

    auto response = GetVersion(billing_tag, OnlineIfNotFound, context);

    if (!response.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "PrefetchPartitions: getting catalog version "
                            "failed, catalog=%s, key=%s",
                            catalog_.ToCatalogHRNString().c_str(), key.c_str());
      callback(response.GetError());
      return;
    }

    auto version = response.GetResult().GetVersion();

    OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchPartitions: catalog=%s, using key=%s",
                       catalog_.ToCatalogHRNString().c_str(), key.c_str());

    repository::PartitionsRepository repository(catalog_, layer_id_, settings_,
                                                lookup_client_, mutex_storage_);

    auto query = [=](std::vector<std::string> partitions,
                     client::CancellationContext inner_context) mutable
        -> PartitionsDataHandleExtendedResponse {
      auto partitions_request = PartitionsRequest()
                                    .WithPartitionIds(std::move(partitions))
                                    .WithBillingTag(billing_tag);
      auto response = repository.GetVersionedPartitionsExtendedResponse(
          partitions_request, version, std::move(inner_context), true);

      if (!response.IsSuccessful()) {
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "PrefetchPartitions: getting versioned "
                              "partitions failed, catalog=%s, key=%s",
                              catalog_.ToCatalogHRNString().c_str(),
                              key.c_str());
        return {response.GetError(), response.GetPayload()};
      }

      PartitionDataHandleResult result;
      std::transform(std::begin(response.GetResult().GetPartitions()),
                     std::end(response.GetResult().GetPartitions()),
                     std::back_inserter(result),
                     [&](const model::Partition& partition) {
                       return std::make_pair(partition.GetPartition(),
                                             partition.GetDataHandle());
                     });

      return {result, response.GetPayload()};
    };

    auto download = [=](std::string data_handle,
                        client::CancellationContext inner_context) mutable {
      if (data_handle.empty()) {
        return BlobApi::DataResponse(
            client::ApiError(client::ErrorCode::NotFound, "Not found"));
      }
      repository::DataCacheRepository data_cache_repository(catalog_,
                                                            settings_.cache);
      if (data_cache_repository.IsCached(layer_id_, data_handle)) {
        data_cache_repository.PromoteInCache(layer_id_, data_handle);
        return BlobApi::DataResponse(nullptr);
      }

      repository::DataRepository repository(catalog_, settings_, lookup_client_,
                                            mutex_storage_);
      // Fetch from online
      return repository.GetVersionedData(
          layer_id_,
          DataRequest()
              .WithDataHandle(std::move(data_handle))
              .WithBillingTag(billing_tag),
          version, std::move(inner_context), true);
    };

    auto append_result = [](const ExtendedDataResponse& response,
                            std::string item,
                            PrefetchPartitionsResult& prefetch_result) {
      if (response.IsSuccessful()) {
        prefetch_result.AddPartition(std::move(item));
      }
    };

    auto call_user_callback = [callback](PrefetchPartitionsResponse result) {
      if (result.IsSuccessful() && result.GetResult().GetPartitions().empty()) {
        callback(ApiError(client::ErrorCode::Unknown,
                          "No partitions were prefetched."));
      } else {
        callback(std::move(result));
      }
    };

    auto download_job = std::make_shared<PrefetchPartitionsHelper::DownloadJob>(
        std::move(download), std::move(append_result),
        std::move(call_user_callback), std::move(status_callback));
    return PrefetchPartitionsHelper::Prefetch(
        std::move(download_job), request.GetPartitionIds(), std::move(query),
        task_sink_, request.GetPriority(), std::move(context));
  };
  const auto priority = request.GetPriority();
  return task_sink_.AddTask(
      std::bind(task, std::placeholders::_1, std::move(request)), priority,
      execution_context);
}

client::CancellableFuture<PrefetchPartitionsResponse>
VersionedLayerClientImpl::PrefetchPartitions(
    PrefetchPartitionsRequest request,
    PrefetchPartitionsStatusCallback status_callback) {
  auto promise = std::make_shared<std::promise<PrefetchPartitionsResponse>>();
  auto cancel_token =
      PrefetchPartitions(std::move(request),
                         [promise](PrefetchPartitionsResponse response) {
                           promise->set_value(std::move(response));
                         },
                         std::move(status_callback));
  return {cancel_token, promise};
}

client::CancellationToken VersionedLayerClientImpl::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback,
    PrefetchStatusCallback status_callback) {
  using client::ApiError;
  using client::ErrorCode;

  client::CancellationContext execution_context;

  execution_context.ExecuteOrCancelled([&]() -> client::CancellationToken {
    return task_sink_.AddTask(
        [=](const client::CancellationContext& context) mutable {
          if (context.IsCancelled()) {
            callback(ApiError::Cancelled());
            return;
          }

          const auto key = request.CreateKey(layer_id_);

          if (!settings_.cache) {
            OLP_SDK_LOG_ERROR_F(kLogTag,
                                "PrefetchPartitions: cache is missing, "
                                "aborting, hrn=%s, key=%s",
                                catalog_.ToString().c_str(), key.c_str());
            callback(ApiError::PreconditionFailed(
                "Unable to prefetch without a cache"));
            return;
          }

          if (request.GetTileKeys().empty()) {
            OLP_SDK_LOG_WARNING_F(
                kLogTag, "PrefetchTiles: invalid request, catalog=%s, key=%s",
                catalog_.ToString().c_str(), key.c_str());
            callback(ApiError::InvalidArgument("Empty tile key list"));
            return;
          }

          auto response =
              GetVersion(request.GetBillingTag(), OnlineIfNotFound, context);

          if (!response.IsSuccessful()) {
            OLP_SDK_LOG_WARNING_F(kLogTag,
                                  "PrefetchTiles: getting catalog version "
                                  "failed, catalog=%s, key=%s",
                                  catalog_.ToString().c_str(), key.c_str());
            callback(response.GetError());
            return;
          }

          auto version = response.GetResult().GetVersion();

          OLP_SDK_LOG_DEBUG_F(kLogTag, "PrefetchTiles: using key=%s",
                              key.c_str());

          // Calculate the minimal set of Tile keys and depth to cover tree
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

          repository::PrefetchTilesRepository repository(
              catalog_, layer_id_, settings_, lookup_client_,
              request.GetBillingTag(), mutex_storage_);

          auto sliced_tiles = repository.GetSlicedTiles(request.GetTileKeys(),
                                                        min_level, max_level);

          if (sliced_tiles.empty()) {
            OLP_SDK_LOG_WARNING_F(
                kLogTag,
                "PrefetchTiles: tile/level mismatch, catalog=%s, key=%s",
                catalog_.ToString().c_str(), key.c_str());
            callback(ApiError::InvalidArgument("TileKeys/levels mismatch"));
            return;
          }

          OLP_SDK_LOG_TRACE_F(kLogTag, "PrefetchTiles: subquads=%zu, key=%s",
                              sliced_tiles.size(), key.c_str());

          const bool aggregation_enabled = request.GetDataAggregationEnabled();

          auto filter = [=](repository::SubQuadsResult& tiles) {
            if (request_only_input_tiles) {
              repository.FilterTilesByList(request, tiles);
            } else {
              repository.FilterTilesByLevel(request, tiles);
            }
          };

          auto query =
              [=](geo::TileKey root,
                  const client::CancellationContext& inner_context) mutable {
                auto response = repository.GetVersionedSubQuads(
                    root, kQuadTreeDepth, version, inner_context);

                if (response.IsSuccessful() && aggregation_enabled) {
                  const auto& tiles = response.GetResult();
                  auto network_stats = repository.LoadAggregatedSubQuads(
                      root,
                      request_only_input_tiles
                          ? repository.FilterTileKeysByList(request, tiles)
                          : repository.FilterTileKeysByLevel(request, tiles),
                      version, inner_context);

                  // append network statistics
                  network_stats += GetNetworkStatistics(response);
                  response = {response.MoveResult(), network_stats};
                }

                return response;
              };

          auto& billing_tag = request.GetBillingTag();

          auto download =
              [=](std::string data_handle,
                  client::CancellationContext inner_context) mutable {
                if (data_handle.empty()) {
                  return BlobApi::DataResponse(
                      ApiError(ErrorCode::NotFound, "Not found"));
                }

                repository::DataCacheRepository cache(catalog_,
                                                      settings_.cache);

                if (cache.IsCached(layer_id_, data_handle)) {
                  cache.PromoteInCache(layer_id_, data_handle);
                  return BlobApi::DataResponse(nullptr);
                }

                repository::DataRepository repository(
                    catalog_, settings_, lookup_client_, mutex_storage_);

                // Fetch from online
                return repository.GetVersionedData(
                    layer_id_,
                    DataRequest()
                        .WithDataHandle(std::move(data_handle))
                        .WithBillingTag(billing_tag),
                    version, std::move(inner_context), true);
              };

          std::vector<geo::TileKey> roots;
          roots.reserve(sliced_tiles.size());

          std::transform(
              sliced_tiles.begin(), sliced_tiles.end(),
              std::back_inserter(roots),
              [](const repository::RootTilesForRequest::value_type& root) {
                return root.first;
              });

          auto append_result = [](const ExtendedDataResponse& response,
                                  geo::TileKey item,
                                  PrefetchTilesResult& prefetch_result) {
            if (response) {
              prefetch_result.emplace_back(std::make_shared<PrefetchTileResult>(
                  item, PrefetchTileNoError()));
            } else {
              prefetch_result.emplace_back(std::make_shared<PrefetchTileResult>(
                  item, response.GetError()));
            }
          };

          auto download_job =
              std::make_shared<PrefetchTilesHelper::DownloadJob>(
                  std::move(download), std::move(append_result),
                  std::move(callback), std::move(status_callback));

          return PrefetchTilesHelper::Prefetch(
              std::move(download_job), roots, std::move(query),
              std::move(filter), task_sink_, request.GetPriority(),
              execution_context);
        },
        request.GetPriority(), client::CancellationContext{});
  });

  return client::CancellationToken(
      [execution_context]() mutable { execution_context.CancelOperation(); });
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
  return {cancel_token, promise};
}

CatalogVersionResponse VersionedLayerClientImpl::GetVersion(
    porting::optional<std::string> billing_tag,
    const FetchOptions& fetch_options,
    const client::CancellationContext& context) {
  auto version = catalog_version_.load();
  if (version != kInvalidVersion) {
    model::VersionResponse response;
    response.SetVersion(version);
    return response;
  }

  CatalogVersionRequest request;
  request.WithBillingTag(std::move(billing_tag));
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
  auto data_task = [=](client::CancellationContext context) -> DataResponse {
    if (request.GetFetchOption() == CacheWithUpdate) {
      return client::ApiError::InvalidArgument(
          "CacheWithUpdate option can not be used for versioned layer");
    }

    if (!request.GetTileKey().IsValid()) {
      return client::ApiError::InvalidArgument("Tile key is invalid");
    }

    auto version_response =
        GetVersion(request.GetBillingTag(), request.GetFetchOption(), context);
    if (!version_response.IsSuccessful()) {
      return version_response.GetError();
    }

    repository::DataRepository repository(catalog_, settings_, lookup_client_,
                                          mutex_storage_);
    return repository.GetVersionedTile(
        layer_id_, request, version_response.GetResult().GetVersion(),
        std::move(context));
  };

  return task_sink_.AddTask(std::move(data_task), std::move(callback),
                            request.GetPriority());
}

client::CancellableFuture<DataResponse> VersionedLayerClientImpl::GetData(
    TileRequest request) {
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto cancel_token =
      GetData(std::move(request), [promise](DataResponse response) {
        promise->set_value(std::move(response));
      });
  return {cancel_token, std::move(promise)};
}

bool VersionedLayerClientImpl::RemoveFromCache(
    const std::string& partition_id) {
  return DeleteFromCache(partition_id).IsSuccessful();
}

bool VersionedLayerClientImpl::RemoveFromCache(const geo::TileKey& tile) {
  return DeleteFromCache(tile).IsSuccessful();
}

client::ApiNoResponse VersionedLayerClientImpl::DeleteFromCache(
    const std::string& partition_id) {
  auto version = catalog_version_.load();
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(
        kLogTag, "Method DeleteFromCache failed, version is not initialized");
    return client::ApiError::PreconditionFailed("Version is not initialized");
  }

  porting::optional<model::Partition> partition;

  repository::PartitionsCacheRepository partitions_cache_repository(
      catalog_, layer_id_, settings_.cache);
  auto clear_response = partitions_cache_repository.ClearPartitionMetadata(
      partition_id, version, partition);
  if (!clear_response) {
    return clear_response;
  }

  if (!partition) {
    return client::ApiNoResult{};
  }

  repository::DataCacheRepository data_cache_repository(catalog_,
                                                        settings_.cache);
  return data_cache_repository.Clear(layer_id_, partition->GetDataHandle());
}

client::ApiNoResponse VersionedLayerClientImpl::DeleteFromCache(
    const geo::TileKey& tile) {
  read::QuadTreeIndex cached_tree;
  repository::PartitionsCacheRepository partitions_cache_repository(
      catalog_, layer_id_, settings_.cache);
  auto version = catalog_version_.load();
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(
        kLogTag, "Method DeleteFromCache failed, version is not initialized");
    return client::ApiError::PreconditionFailed("Version is not initialized");
  }

  if (!partitions_cache_repository.FindQuadTree(tile, version, cached_tree)) {
    return client::ApiNoResult{};
  }

  auto data = cached_tree.Find(tile, false);
  if (!data) {
    return client::ApiNoResult{};
  }
  repository::DataCacheRepository data_cache_repository(catalog_,
                                                        settings_.cache);
  auto result = data_cache_repository.Clear(layer_id_, data->data_handle);
  if (!result) {
    return result;
  }

  auto index_data = cached_tree.GetIndexData(QuadTreeIndex::DataHandle);
  for (const auto& ind : index_data) {
    if (ind.tile_key != tile &&
        data_cache_repository.IsCached(layer_id_, ind.data_handle)) {
      return client::ApiNoResult{};
    }
  }

  return partitions_cache_repository.ClearQuadTree(cached_tree.GetRootTile(),
                                                   kQuadTreeDepth, version);
}

bool VersionedLayerClientImpl::IsCached(const std::string& partition_id) {
  auto version = catalog_version_.load();
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "Method IsCached failed, version is not initialized");
    return false;
  }

  auto cache = settings_.cache;

  repository::PartitionsCacheRepository partitions_repo(catalog_, layer_id_,
                                                        cache);

  std::string handle;
  if (!partitions_repo.GetPartitionHandle(partition_id, version, handle)) {
    return false;
  }

  repository::DataCacheRepository data_repo(catalog_, cache);
  return data_repo.IsCached(layer_id_, handle);
}

bool VersionedLayerClientImpl::IsCached(const geo::TileKey& tile,
                                        bool aggregated) {
  auto version = catalog_version_.load();
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "Method IsCached failed, version is not initialized");
    return false;
  }

  read::QuadTreeIndex cached_tree;

  auto cache = settings_.cache;

  repository::PartitionsCacheRepository partitions_repo(catalog_, layer_id_,
                                                        cache);

  if (!partitions_repo.FindQuadTree(tile, version, cached_tree)) {
    return false;
  }

  auto data = cached_tree.Find(tile, aggregated);
  if (!data) {
    return false;
  }

  repository::DataCacheRepository data_repo(catalog_, cache);
  return data_repo.IsCached(layer_id_, data->data_handle);
}

client::CancellationToken VersionedLayerClientImpl::GetAggregatedData(
    TileRequest request, AggregatedDataResponseCallback callback) {
  auto data_task = [=](const client::CancellationContext& context)
      -> AggregatedDataResponse {
    const auto fetch_option = request.GetFetchOption();
    const auto& billing_tag = request.GetBillingTag();

    if (fetch_option == CacheWithUpdate) {
      return client::ApiError::InvalidArgument(
          "CacheWithUpdate option can not be used for versioned layer");
    }

    if (!request.GetTileKey().IsValid()) {
      return client::ApiError::InvalidArgument("Tile key is invalid");
    }

    auto version_response = GetVersion(billing_tag, fetch_option, context);
    if (!version_response.IsSuccessful()) {
      return version_response.GetError();
    }

    auto version = version_response.GetResult().GetVersion();
    repository::PartitionsRepository partition_repository(
        catalog_, layer_id_, settings_, lookup_client_, mutex_storage_);
    const auto& partition_response =
        partition_repository.GetAggregatedTile(request, version, context);
    if (!partition_response.IsSuccessful()) {
      return {partition_response.GetError(), partition_response.GetPayload()};
    }

    const auto& partition = partition_response.GetResult();

    repository::DataRepository data_repository(catalog_, settings_,
                                               lookup_client_, mutex_storage_);
    auto data_response = data_repository.GetBlobData(
        layer_id_, "blob", partition, fetch_option, billing_tag, context,
        settings_.propagate_all_cache_errors);

    const auto aggregated_network_statistics =
        partition_response.GetPayload() + data_response.GetPayload();

    if (!data_response) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "GetAggregatedData: failed to load data, key=%s, data_handle=%s",
          partition.GetPartition().c_str(), partition.GetDataHandle().c_str());
      return {data_response.GetError(), aggregated_network_statistics};
    }

    AggregatedDataResult result;
    result.SetTile(geo::TileKey::FromHereTile(partition.GetPartition()));
    result.SetData(data_response.MoveResult());

    return {result, aggregated_network_statistics};
  };

  return task_sink_.AddTask(std::move(data_task), std::move(callback),
                            request.GetPriority());
}

client::CancellableFuture<AggregatedDataResponse>
VersionedLayerClientImpl::GetAggregatedData(TileRequest request) {
  auto promise = std::make_shared<std::promise<AggregatedDataResponse>>();
  auto cancel_token = GetAggregatedData(
      std::move(request), [promise](AggregatedDataResponse response) {
        promise->set_value(std::move(response));
      });
  return {cancel_token, std::move(promise)};
}

bool VersionedLayerClientImpl::Protect(const TileKeys& tiles) {
  if (!settings_.cache) {
    return false;
  }
  auto version = catalog_version_.load();
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "Method Protect failed, version is not initialized");
    return false;
  }

  auto tiles_dependency_resolver =
      ProtectDependencyResolver(catalog_, layer_id_, version, settings_);
  const auto& keys_to_protect =
      tiles_dependency_resolver.GetKeysToProtect(tiles);

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
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "Method Release failed, version is not initialized");
    return false;
  }

  auto tiles_dependency_resolver =
      ReleaseDependencyResolver(catalog_, layer_id_, version, settings_);
  const auto& keys_to_release =
      tiles_dependency_resolver.GetKeysToRelease(tiles);

  if (keys_to_release.empty()) {
    return false;
  }

  return settings_.cache->Release(keys_to_release);
}

bool VersionedLayerClientImpl::Protect(
    const std::vector<std::string>& partition_ids) {
  if (!settings_.cache) {
    return {};
  }

  auto version = catalog_version_.load();
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "Method Protect failed, version is not initialized");
    return {};
  }

  repository::PartitionsCacheRepository repository(catalog_, layer_id_,
                                                   settings_.cache);

  return repository.Protect(partition_ids, version);
}

bool VersionedLayerClientImpl::Release(
    const std::vector<std::string>& partition_ids) {
  if (!settings_.cache) {
    return {};
  }

  auto version = catalog_version_.load();
  if (version == kInvalidVersion) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "Method Release failed, version is not initialized");
    return {};
  }

  repository::PartitionsCacheRepository repository(catalog_, layer_id_,
                                                   settings_.cache);

  return repository.Release(partition_ids, version);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
