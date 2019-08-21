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

using namespace olp::client;

PrefetchTilesProvider::PrefetchTilesProvider(
    const HRN& /*hrn*/, std::shared_ptr<repository::ApiRepository> apiRepo,
    std::shared_ptr<repository::CatalogRepository> catalogRepo,
    std::shared_ptr<repository::DataRepository> dataRepo,
    std::shared_ptr<repository::PrefetchTilesRepository> prefetchTilesRepo,
    std::shared_ptr<olp::client::OlpClientSettings> settings)
    : apiRepo_(std::move(apiRepo)),
      catalogRepo_(std::move(catalogRepo)),
      dataRepo_(std::move(dataRepo)),
      prefetchTilesRepo_(std::move(prefetchTilesRepo)),
      settings_(std::move(settings)) {
  prefetchProviderBusy_ = std::make_shared<std::atomic_bool>(false);
}

client::CancellationToken PrefetchTilesProvider::PrefetchTiles(
    const PrefetchTilesRequest& request,
    const PrefetchTilesResponseCallback& callback) {
  auto key = request.CreateKey();
  EDGE_SDK_LOG_TRACE_F(kLogTag, "getCatalog(%s)", key.c_str());
  auto isBusy = prefetchProviderBusy_->exchange(true);
  if (isBusy) {
    repository::ExecuteOrSchedule(settings_.get(), [=]() {
      EDGE_SDK_LOG_INFO_F(kLogTag, "getCatalog(%s) busy", key.c_str());
      callback(
          {{ErrorCode::SlowDown, "Busy prefetching at the moment.", true}});
    });
    return client::CancellationToken();
  }

  auto busy = prefetchProviderBusy_;
  auto completionCallback = [=](const PrefetchTilesResponse& response) {
    busy->store(false);
    callback(response);
  };

  auto cancel_context = std::make_shared<CancellationContext>();

  auto cancel_callback = [completionCallback, key]() {
    EDGE_SDK_LOG_INFO_F(kLogTag, "getCatalog(%s) cancelled", key.c_str());
    completionCallback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  auto apiRepo = apiRepo_;
  auto catalogRepo = catalogRepo_;
  auto dataRepo = dataRepo_;
  auto prefetchTilesRepo = prefetchTilesRepo_;

  // Get the catalog (and layers) config
  CatalogRequest catalogRequest;
  catalogRequest.WithBillingTag(request.GetBillingTag());
  cancel_context->ExecuteOrCancelled(
      [=]() {
        EDGE_SDK_LOG_INFO_F(kLogTag, "getCatalog(%s) execute", key.c_str());

        return catalogRepo->getCatalog(
            catalogRequest, [=](read::CatalogResponse catalogResponse) {
              if (!catalogResponse.IsSuccessful()) {
                EDGE_SDK_LOG_INFO_F(kLogTag, "getCatalog(%s) unsuccessful",
                                    key.c_str());
                completionCallback(catalogResponse.GetError());
                return;
              }

              auto layers = catalogResponse.GetResult().GetLayers();
              auto layerResult = std::find_if(
                  layers.begin(), layers.end(), [request](model::Layer layer) {
                    return (layer.GetId().compare(request.GetLayerId()) == 0);
                  });
              if (layerResult == layers.end()) {
                // Layer not found
                EDGE_SDK_LOG_INFO_F(
                    kLogTag, "getLatestCatalogVersion(%s) layer not found",
                    key.c_str());
                completionCallback(ApiError(client::ErrorCode::InvalidArgument,
                                            "Layer specified doesn't exist."));
                return;
              }
              auto layerType = (*layerResult).GetLayerType();
              boost::optional<time_t> expiry;
              if ((*layerResult).GetTtl()) {
                expiry = *(*layerResult).GetTtl() / 1000;
              }

              // Get the catalog version
              cancel_context->ExecuteOrCancelled(
                  [=]() {
                    EDGE_SDK_LOG_INFO_F(kLogTag,
                                        "getLatestCatalogVersion(%s) execute",
                                        key.c_str());
                    CatalogVersionRequest catalogVersionRequest;
                    catalogVersionRequest
                        .WithBillingTag(request.GetBillingTag())
                        .WithStartVersion(-1);
                    return catalogRepo->getLatestCatalogVersion(
                        catalogVersionRequest,
                        [=](CatalogVersionResponse response) {
                          if (!response.IsSuccessful()) {
                            EDGE_SDK_LOG_INFO_F(
                                kLogTag,
                                "getLatestCatalogVersion(%s) unseccessful",
                                key.c_str());
                            completionCallback(response.GetError());
                            return;
                          }

                          auto version = response.GetResult().GetVersion();

                          // Calculate the minimal set of Tile keys and depth to
                          // cover tree.
                          auto calculatedTileKeys = repository::
                              PrefetchTilesRepository::EffectiveTileKeys(
                                  request.GetTileKeys(), request.GetMinLevel(),
                                  request.GetMaxLevel());

                          if (calculatedTileKeys.size() == 0) {
                            EDGE_SDK_LOG_INFO_F(
                                kLogTag,
                                "getLatestCatalogVersion(%s) tile/level "
                                "mismatch",
                                key.c_str());
                            completionCallback(
                                {{client::ErrorCode::InvalidArgument,
                                  "TileKey and Levels mismatch."}});
                            return;
                          }

                          EDGE_SDK_LOG_INFO_F(kLogTag,
                                              "EffectiveTileKeys, count = %lu",
                                              calculatedTileKeys.size());
                          prefetchTilesRepo->GetSubTiles(
                              cancel_context, request, version, expiry,
                              calculatedTileKeys,
                              [=](const repository::SubTilesResponse&
                                      response) {
                                if (!response.IsSuccessful()) {
                                  EDGE_SDK_LOG_INFO_F(
                                      kLogTag,
                                      "SubTilesResponse(%s) unseccessful",
                                      key.c_str());
                                  completionCallback({response.GetError()});
                                  return;
                                }

                                // Query for each of the tiles' data handle (or
                                // just embedded data)
                                QueryDataForEachSubTile(
                                    cancel_context, dataRepo, request,
                                    layerType, response.GetResult(),
                                    completionCallback);
                              });
                        });
                  },
                  cancel_callback);
            });
      },
      cancel_callback);

  return CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
}

void PrefetchTilesProvider::QueryDataForEachSubTile(
    std::shared_ptr<client::CancellationContext> cancel_context,
    std::shared_ptr<repository::DataRepository> dataRepo,
    const PrefetchTilesRequest& request, const std::string& layerType,
    const repository::SubTilesResult& subtiles,
    const PrefetchTilesResponseCallback& callback) {
  EDGE_SDK_LOG_TRACE_F(kLogTag, "QueryDataForEachSubTile, count = %lu",
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
            .WithLayerId(request.GetLayerId())
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
        dataRepo->GetData(cancel_context, layerType, dataRequest, dataCallback);
        return p->get_future().get();
      }
    }));
  }

  auto result = std::make_shared<PrefetchTilesResult>();
  for (auto& f : futures) {
    auto r = f.get();
    result->push_back(r);
  }

  callback({*result});
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
