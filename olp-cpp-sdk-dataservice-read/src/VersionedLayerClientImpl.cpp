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
#include <olp/core/logging/Log.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include "Common.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/PartitionsRepository.h"
#include "repositories/PrefetchTilesRepository.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "VersionedLayerClientImpl";
constexpr int64_t kInvalidVersion = -1;
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
                                       : kInvalidVersion) {
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
  if (request.GetVersion()) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "PartitionsRequest::WithVersion() is deprecated. Use "
                        "VersionedLayerClient constructor to specify version");
  }

  auto schedule_get_partitions = [&](PartitionsRequest request,
                                     PartitionsResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;

    auto partitions_task =
        [=](client::CancellationContext context) mutable -> PartitionsResponse {
      auto version_response = GetVersion(request.GetBillingTag(),
                                         request.GetFetchOption(), context);
      if (!version_response.IsSuccessful()) {
        return version_response.GetError();
      }
      request.WithVersion(version_response.GetResult().GetVersion());

      return repository::PartitionsRepository::GetVersionedPartitions(
          catalog, layer_id, context, std::move(request), settings);
    };

    return AddTask(settings.task_scheduler, pending_requests_,
                   std::move(partitions_task), std::move(callback));
  };

  if (request.GetFetchOption() == CacheWithUpdate) {
    request.WithFetchOption(FetchOptions::OnlineIfNotFound);
    return ScheduleFetch(
        [&](PartitionsRequest, PartitionsResponseCallback callback) {
          return AddTask(
              settings_.task_scheduler, pending_requests_,
              [=](client::CancellationContext) -> PartitionsResponse {
                return {{client::ErrorCode::InvalidArgument,
                         "CacheWithUpdate option can not be used for versioned "
                         "layer."}};
              },
              std::move(callback));
        },
        std::move(request), std::move(callback));
  }

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
  if (request.GetVersion()) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "DataRequest::WithVersion() is deprecated. Use "
                        "VersionedLayerClient constructor to specify version");
  }

  auto schedule_get_data = [&](DataRequest request,
                               DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;

    auto data_task =
        [=](client::CancellationContext context) mutable -> DataResponse {
      if (!request.GetDataHandle()) {
        auto version_response = GetVersion(request.GetBillingTag(),
                                           request.GetFetchOption(), context);
        if (!version_response.IsSuccessful()) {
          return version_response.GetError();
        }
        request.WithVersion(version_response.GetResult().GetVersion());
      }

      return repository::DataRepository::GetVersionedData(
          std::move(catalog), std::move(layer_id), std::move(request), context,
          std::move(settings));
    };

    return AddTask(settings.task_scheduler, pending_requests_,
                   std::move(data_task), std::move(callback));
  };

  if (request.GetFetchOption() == CacheWithUpdate) {
    request.WithFetchOption(FetchOptions::OnlineIfNotFound);
    return ScheduleFetch(
        [&](DataRequest, DataResponseCallback callback) {
          return AddTask(
              settings_.task_scheduler, pending_requests_,
              [=](client::CancellationContext) -> DataResponse {
                return {{client::ErrorCode::InvalidArgument,
                         "CacheWithUpdate option can not be used for versioned "
                         "layer."}};
              },
              std::move(callback));
        },
        std::move(request), std::move(callback));
  }
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
  // Used as empty response to be able to execute initial task
  using EmptyResponse = Response<PrefetchTileNoError>;
  using PrefetchResult = PrefetchTilesResult::value_type;
  using PrefetchResultFuture = std::future<PrefetchResult>;
  using PrefetchResultPromise = std::promise<PrefetchResult>;
  using client::CancellationContext;
  using client::ErrorCode;

  auto catalog = catalog_;
  auto layer_id = layer_id_;
  auto settings = settings_;
  auto pending_requests = pending_requests_;

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

        request.WithVersion(response.GetResult().GetVersion());

        const auto key = request.CreateKey(layer_id);
        OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles: using key=%s", key.c_str());

        // Calculate the minimal set of Tile keys and depth to
        // cover tree.
        auto sub_quads = repository::PrefetchTilesRepository::EffectiveTileKeys(
            request.GetTileKeys(), request.GetMinLevel(),
            request.GetMaxLevel());

        if (sub_quads.empty()) {
          OLP_SDK_LOG_WARNING_F(kLogTag,
                                "PrefetchTiles: tile/level mismatch, key=%s",
                                key.c_str());
          return {{ErrorCode::InvalidArgument, "TileKeys/levels mismatch"}};
        }

        OLP_SDK_LOG_DEBUG_F(kLogTag, "PrefetchTiles, subquads=%zu, key=%s",
                            sub_quads.size(), key.c_str());

        // Now get metadata for subtiles using QueryApi::QuadTreeIndex
        auto sub_tiles = repository::PrefetchTilesRepository::GetSubTiles(
            catalog, layer_id, request, sub_quads, context, settings);

        if (!sub_tiles.IsSuccessful()) {
          return sub_tiles.GetError();
        }

        const auto& tiles_result = sub_tiles.GetResult();
        if (tiles_result.empty()) {
          OLP_SDK_LOG_WARNING_F(
              kLogTag, "PrefetchTiles: subtiles empty, key=%s", key.c_str());
          return {{ErrorCode::InvalidArgument, "Subquads retrieval failed"}};
        }

        OLP_SDK_LOG_INFO_F(kLogTag, "Prefetch start, key=%s, tiles=%zu",
                           key.c_str(), tiles_result.size());

        // Once we have the data create for each subtile a task and push it
        // onto the TaskScheduler. One additional last task is added which
        // waits for all previous tasks to finish so that it may call the user
        // with the result.
        auto futures = std::make_shared<std::vector<PrefetchResultFuture>>();
        std::vector<CancellationContext> contexts;
        contexts.reserve(tiles_result.size() + 1u);
        auto it = tiles_result.begin();

        while (!context.IsCancelled() && it != tiles_result.end()) {
          auto promise = std::make_shared<PrefetchResultPromise>();
          auto flag = std::make_shared<std::atomic_bool>(false);
          futures->emplace_back(promise->get_future());

          auto tile = it->first;
          auto handle = it->second;
          auto context_it = contexts.emplace(contexts.end());

          AddTask(settings.task_scheduler, pending_requests,
                  [=](CancellationContext inner_context) {
                    // Get blob data
                    auto data = repository::DataRepository::GetVersionedData(
                        catalog, layer_id,
                        DataRequest().WithDataHandle(handle).WithBillingTag(
                            request.GetBillingTag()),
                        inner_context, settings);

                    if (!data.IsSuccessful()) {
                      promise->set_value(std::make_shared<PrefetchTileResult>(
                          tile, data.GetError()));
                    } else {
                      promise->set_value(std::make_shared<PrefetchTileResult>(
                          tile, PrefetchTileNoError()));
                    }

                    flag->exchange(true);
                    return EmptyResponse(PrefetchTileNoError());
                  },
                  [=](EmptyResponse result) {
                    if (!flag->load()) {
                      // If above task was cancelled we might need to set
                      // promise else below task will wait forever
                      promise->set_value(std::make_shared<PrefetchTileResult>(
                          tile,
                          client::ApiError(ErrorCode::Cancelled, "Cancelled")));
                    }
                  },
                  *context_it);
          it++;
        }

        // Task to wait for previously triggered data download to collect
        // responses and trigger user callback.
        AddTask(
            settings.task_scheduler, pending_requests,
            [=](CancellationContext inner_context) -> PrefetchTilesResponse {
              PrefetchTilesResult result;
              result.reserve(futures->size());

              for (auto& future : *futures) {
                // Check if cancelled in between.
                if (inner_context.IsCancelled()) {
                  return {{ErrorCode::Cancelled, "Cancelled"}};
                }

                auto tile_result = future.get();
                result.emplace_back(std::move(tile_result));
              }

              OLP_SDK_LOG_INFO_F(kLogTag, "Prefetch done, key=%s, tiles=%zu",
                                 key.c_str(), result.size());
              return PrefetchTilesResponse(std::move(result));
            },
            callback, *contexts.emplace(contexts.end()));

        context.ExecuteOrCancelled([&]() {
          return client::CancellationToken([contexts]() {
            for (auto context : contexts) {
              context.CancelOperation();
            }
          });
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
}  // namespace read

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
  auto response = repository::CatalogRepository::GetLatestVersion(
      catalog_, context, request, settings_);

  if (!response.IsSuccessful()) {
    return response;
  }

  if (!catalog_version_.compare_exchange_weak(
          version, response.GetResult().GetVersion())) {
    model::VersionResponse response;
    response.SetVersion(version);
    return response;
  }

  return response;
}

client::CancellationToken VersionedLayerClientImpl::GetData(
    TileRequest request, DataResponseCallback callback) {
  auto schedule_get_data = [&](TileRequest request,
                               DataResponseCallback callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;
    auto pending_requests = pending_requests_;

    auto data_task = [=](client::CancellationContext context) -> DataResponse {
      auto version_response = GetVersion(request.GetBillingTag(),
                                         request.GetFetchOption(), context);
      if (!version_response.IsSuccessful()) {
        return version_response.GetError();
      }

      return repository::DataRepository::GetVersionedTile(
          std::move(catalog), std::move(layer_id), std::move(request),
          version_response.GetResult().GetVersion(), context,
          std::move(settings));
    };

    return AddTask(settings.task_scheduler, pending_requests,
                   std::move(data_task), std::move(callback));
  };

  if (request.GetFetchOption() == CacheWithUpdate) {
    request.WithFetchOption(FetchOptions::OnlineIfNotFound);
    return ScheduleFetch(
        [&](TileRequest, DataResponseCallback callback) {
          return AddTask(
              settings_.task_scheduler, pending_requests_,
              [=](client::CancellationContext) -> DataResponse {
                return {{client::ErrorCode::InvalidArgument,
                         "CacheWithUpdate option can not be used for versioned "
                         "layer."}};
              },
              std::move(callback));
        },
        std::move(request), std::move(callback));
  }

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

}  // namespace read
}  // namespace dataservice
}  // namespace olp
