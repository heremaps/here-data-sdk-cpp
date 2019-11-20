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

#include "PrefetchTilesProvider.h"

#include <olp/core/client/CancellationContext.h>
#include <olp/core/logging/Log.h>

#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PrefetchTileResult.h"
#include "olp/dataservice/read/PrefetchTilesRequest.h"

#include "repositories/ApiRepository.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PrefetchTilesRepository.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "PrefetchTilesProvider";
}  // namespace

PrefetchTilesProvider::PrefetchTilesProvider(
    const client::HRN& /*hrn*/, std::string layer_id,
    std::shared_ptr<repository::ApiRepository> api_repo,
    std::shared_ptr<repository::CatalogRepository> catalog_repo,
    std::shared_ptr<repository::DataRepository> data_repo,
    std::shared_ptr<repository::PrefetchTilesRepository> prefetch_tiles_repo,
    std::shared_ptr<client::OlpClientSettings> settings)
    : prefetch_provider_busy_(std::make_shared<std::atomic_bool>(false)),
      api_repo_(std::move(api_repo)),
      catalog_repo_(std::move(catalog_repo)),
      data_repo_(std::move(data_repo)),
      prefetch_tiles_repo_(std::move(prefetch_tiles_repo)),
      settings_(std::move(settings)),
      layer_id_(std::move(layer_id)) {}

client::CancellationToken PrefetchTilesProvider::PrefetchTiles(
    const PrefetchTilesRequest& request,
    const PrefetchTilesResponseCallback& callback) {
  auto key = request.CreateKey(layer_id_);
  OLP_SDK_LOG_TRACE_F(kLogTag, "PrefetchTiles(%s)", key.c_str());

  if (prefetch_provider_busy_->exchange(true)) {
    repository::ExecuteOrSchedule(settings_.get(), [=]() {
      OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles(%s) busy", key.c_str());
      callback({{client::ErrorCode::SlowDown, "Busy prefetching at the moment.",
                 true}});
    });
    return client::CancellationToken();
  }

  auto busy = prefetch_provider_busy_;
  auto completion_callback = [=](PrefetchTilesResponse response) {
    busy->store(false);
    callback(std::move(response));
  };

  client::CancellationContext cancel_context;

  auto cancel_callback = [=]() {
    OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles(%s) cancelled", key.c_str());
    completion_callback(
        {{client::ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  auto api_repo = api_repo_;
  auto catalog_repo = catalog_repo_;
  auto data_repo = data_repo_;
  auto prefetch_tiles_repo = prefetch_tiles_repo_;
  auto layer_id = layer_id_;

  // Get the catalog (and layers) config
  CatalogRequest catalog_request;
  catalog_request.WithBillingTag(request.GetBillingTag());

  cancel_context.ExecuteOrCancelled(
      [=]() mutable {
        OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles(%s) execute", key.c_str());

        return catalog_repo->getCatalog(
            catalog_request,
            [=](read::CatalogResponse catalog_response) mutable {
              if (!catalog_response.IsSuccessful()) {
                OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles(%s) unsuccessful",
                                   key.c_str());
                completion_callback(catalog_response.GetError());
                return;
              }

              auto layers = catalog_response.GetResult().GetLayers();
              auto layerResult = std::find_if(
                  layers.begin(), layers.end(), [layer_id](model::Layer layer) {
                    return (layer.GetId().compare(layer_id) == 0);
                  });
              if (layerResult == layers.end()) {
                // Layer not found
                OLP_SDK_LOG_INFO_F(kLogTag, "PrefetchTiles(%s) layer not found",
                                   key.c_str());
                completion_callback({{client::ErrorCode::InvalidArgument,
                                      "Layer specified doesn't exist."}});
                return;
              }
              auto layerType = (*layerResult).GetLayerType();
              boost::optional<time_t> expiry;
              if ((*layerResult).GetTtl()) {
                expiry = *(*layerResult).GetTtl() / 1000;
              }

              // Get the catalog version
              cancel_context.ExecuteOrCancelled(
                  [=]() mutable {
                    OLP_SDK_LOG_INFO_F(kLogTag,
                                       "getLatestCatalogVersion(%s) execute",
                                       key.c_str());
                    CatalogVersionRequest catalogVersionRequest;
                    catalogVersionRequest
                        .WithBillingTag(request.GetBillingTag())
                        .WithStartVersion(-1);
                    return catalog_repo->getLatestCatalogVersion(
                        catalogVersionRequest,
                        [=](CatalogVersionResponse response) mutable {
                          if (!response.IsSuccessful()) {
                            OLP_SDK_LOG_INFO_F(
                                kLogTag,
                                "getLatestCatalogVersion(%s) unseccessful",
                                key.c_str());
                            completion_callback(response.GetError());
                            return;
                          }

                          auto version = response.GetResult().GetVersion();

                          // Calculate the minimal set of Tile keys and depth to
                          // cover tree.
                          auto calculated_tile_keys = repository::
                              PrefetchTilesRepository::EffectiveTileKeys(
                                  request.GetTileKeys(), request.GetMinLevel(),
                                  request.GetMaxLevel());

                          if (calculated_tile_keys.empty()) {
                            OLP_SDK_LOG_INFO_F(
                                kLogTag,
                                "getLatestCatalogVersion(%s) tile/level "
                                "mismatch",
                                key.c_str());
                            completion_callback(
                                {{client::ErrorCode::InvalidArgument,
                                  "TileKey and Levels mismatch."}});
                            return;
                          }

                          OLP_SDK_LOG_INFO_F(kLogTag,
                                             "EffectiveTileKeys, count = %lu",
                                             calculated_tile_keys.size());
                          prefetch_tiles_repo->GetSubTiles(
                              nullptr, request, version, expiry,
                              calculated_tile_keys,
                              [=](const repository::SubTilesResponse&
                                      response) mutable {
                                if (!response.IsSuccessful()) {
                                  OLP_SDK_LOG_INFO_F(
                                      kLogTag,
                                      "SubTilesResponse(%s) unseccessful",
                                      key.c_str());
                                  completion_callback({response.GetError()});
                                  return;
                                }

                                // Query for each of the tiles' data handle (or
                                // just embedded data)
                                QueryDataForEachSubTile(
                                    cancel_context, data_repo, request,
                                    layerType, response.GetResult(),
                                    completion_callback);
                              });
                        });
                  },
                  cancel_callback);
            });
      },
      cancel_callback);

  return client::CancellationToken(
      [=]() mutable { cancel_context.CancelOperation(); });
}

void PrefetchTilesProvider::QueryDataForEachSubTile(
    client::CancellationContext context,
    std::shared_ptr<repository::DataRepository> data_repo,
    const PrefetchTilesRequest& request, const std::string& layer_type,
    const repository::SubTilesResult& subtiles,
    const PrefetchTilesResponseCallback& callback) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "QueryDataForEachSubTile, count = %lu",
                      subtiles.size());

  std::vector<std::future<std::shared_ptr<PrefetchTileResult>>> futures;

  // Query for each of the tiles' data
  for (auto subtile : subtiles) {
    futures.push_back(std::async([=]() {
      // Check for embedded data
      if (repository::DataRepository::IsInlineData(subtile.second)) {
        return std::make_shared<PrefetchTileResult>(subtile.first,
                                                    PrefetchTileNoError());
      } else {
        DataRequest dataRequest;
        dataRequest.WithDataHandle(subtile.second)
            .WithBillingTag(request.GetBillingTag());

        auto p = std::make_shared<
            std::promise<std::shared_ptr<PrefetchTileResult>>>();

        auto dataCallback = [=](DataResponse response) {
          // add Tile response.
          std::shared_ptr<PrefetchTileResult> r;
          if (!response.IsSuccessful()) {
            p->set_value(std::make_shared<PrefetchTileResult>(
                subtile.first, response.GetError()));
          } else {
            p->set_value(std::make_shared<PrefetchTileResult>(
                subtile.first, PrefetchTileNoError()));
          }
        };
        // Disabled currently to be able to transition to raw
        // CancellationContext. Will be remove in next commit.
        // data_repo->GetData(context, layer_type, dataRequest, dataCallback);
        return p->get_future().get();
      }
    }));
  }

  auto result = PrefetchTilesResult();
  result.reserve(futures.size());

  for (auto& f : futures) {
    result.push_back(f.get());
  }

  callback({std::move(result)});
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
