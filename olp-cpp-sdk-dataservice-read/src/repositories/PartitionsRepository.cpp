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

#include "PartitionsRepository.h"

#include <algorithm>
#include <sstream>

#include <olp/core/logging/Log.h>

#include "ApiRepository.h"
#include "CatalogRepository.h"
#include "ExecuteOrSchedule.inl"
#include "PartitionsCacheRepository.h"
#include "generated/api/MetadataApi.h"
#include "generated/api/QueryApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"


namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;

namespace {
constexpr auto kLogTag = "PartitionsRepository";

using LayerVersionReponse = ApiResponse<int64_t, client::ApiError>;
using LayerVersionCallback = std::function<void(LayerVersionReponse)>;

std::string GetKey(const PartitionsRequest& request,
                   const std::vector<std::string>& partitions) {
  std::stringstream ss;
  ss << request.GetLayerId();

  ss << "[";
  bool first = true;
  for (auto& id : partitions) {
    if (!first) {
      ss << ",";
    } else {
      first = false;
    }
    ss << id;
  }
  ss << "]";

  if (request.GetVersion()) {
    ss << "@" << request.GetVersion().get();
  }

  if (request.GetBillingTag()) {
    ss << "$" << request.GetBillingTag().get();
  }

  ss << "^" << request.GetFetchOption();

  return ss.str();
}

void GetLayerVersion(std::shared_ptr<CancellationContext> cancel_context,
                     const OlpClient& client, const PartitionsRequest& request,
                     const LayerVersionCallback& callback,
                     PartitionsCacheRepository& cache,
                     std::shared_ptr<ApiRepository> apiRepo) {
  auto key = request.CreateKey();
  auto layerVersionsCallback = [=](model::LayerVersions layerVersions) {
    auto& versionLayers = layerVersions.GetLayerVersions();
    auto itr = std::find_if(versionLayers.begin(), versionLayers.end(),
                            [&](const model::LayerVersion& layer) {
                              return layer.GetLayer() == request.GetLayerId();
                            });

    if (itr != versionLayers.end()) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "version for '%s' is '%ld'", key.c_str(),
                          itr->GetVersion());
      callback(itr->GetVersion());
    } else {
      EDGE_SDK_LOG_INFO_F(kLogTag, "version for '%s' not found", key.c_str());
      callback(ApiError(client::ErrorCode::InvalidArgument,
                        "Layer specified doesn't exist."));
    }
  };

  auto cancel_callback = [callback, key]() {
    EDGE_SDK_LOG_INFO_F(kLogTag, "Put '%s'", key.c_str());
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  cancel_context->ExecuteOrCancelled(
      [=, &cache]() {
        auto cachedLayerVersions = cache.Get(*request.GetVersion());
        if (cachedLayerVersions) {
          ExecuteOrSchedule(apiRepo->GetOlpClientSettings(),
              [=]() {
                EDGE_SDK_LOG_INFO_F(kLogTag, "cache parititions '%s' found!",
                                    key.c_str());
                layerVersionsCallback(*cachedLayerVersions);
              }
            );
          return CancellationToken();
        }
        return MetadataApi::GetLayerVersions(
            client, *request.GetVersion(), request.GetBillingTag(),
            [=, &cache](MetadataApi::LayerVersionsResponse response) {
              if (!response.IsSuccessful()) {
                EDGE_SDK_LOG_INFO_F(kLogTag, "GetLayerVersions '%s' not found!",
                                    key.c_str());
                callback(response.GetError());
                return;
              }
              // Cache the results
              EDGE_SDK_LOG_INFO_F(kLogTag, "GetLayerVersions '%s' found!",
                                  key.c_str());
              cache.Put(*request.GetVersion(), response.GetResult());
              layerVersionsCallback(response.GetResult());
            });
      },

      cancel_callback);
}

void appendPartitionsRequest(
    const PartitionsRequest& request,
    const std::shared_ptr<CatalogRepository>& catalogRepo,
    const std::shared_ptr<CancellationContext>& cancel_context,
    const std::function<void()>& cancel_callback,
    const PartitionsResponseCallback& callback,
    const std::function<void(const PartitionsRequest&,
                             const boost::optional<time_t>&)>&
        fetchPartitions) {
  CatalogRequest catalogRequest;
  catalogRequest.WithBillingTag(request.GetBillingTag())
      .WithFetchOption(request.GetFetchOption());
  cancel_context->ExecuteOrCancelled(
      [=]() {
        return catalogRepo->getCatalog(
            catalogRequest, [=](read::CatalogResponse catalogResponse) {
              if (!catalogResponse.IsSuccessful()) {
                callback(catalogResponse.GetError());
                return;
              }

              auto& catalogLayers = catalogResponse.GetResult().GetLayers();
              auto itr =
                  std::find_if(catalogLayers.begin(), catalogLayers.end(),
                               [&](const model::Layer& layer) {
                                 return layer.GetId() == request.GetLayerId();
                               });

              if (itr == catalogLayers.end()) {
                callback(ApiError(client::ErrorCode::InvalidArgument,
                                  "Layer specified doesn't exist."));
                return;
              }

              auto layerType = itr->GetLayerType();
              boost::optional<time_t> expiry;
              if (itr->GetTtl()) {
                expiry = *itr->GetTtl() / 1000;
              }

              if (layerType != "versioned") {
                auto appendedRequest = request;
                appendedRequest.WithVersion(boost::none);
                fetchPartitions(appendedRequest, expiry);
              } else if (request.GetVersion()) {
                fetchPartitions(request, expiry);
              } else {
                cancel_context->ExecuteOrCancelled(
                    [=]() {
                      CatalogVersionRequest catalogVersionRequest;
                      catalogVersionRequest
                          .WithBillingTag(request.GetBillingTag())
                          .WithFetchOption(request.GetFetchOption())
                          .WithStartVersion(-1);
                      return catalogRepo->getLatestCatalogVersion(
                          catalogVersionRequest,
                          [=](CatalogVersionResponse response) {
                            if (!response.IsSuccessful()) {
                              callback(response.GetError());
                              return;
                            }

                            auto appendedRequest = request;
                            appendedRequest.WithVersion(
                                response.GetResult().GetVersion());
                            fetchPartitions(appendedRequest, expiry);
                          });
                    },
                    cancel_callback);
              }
            });
      },
      cancel_callback);
}

}  // namespace

PartitionsRepository::PartitionsRepository(
    const HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
    std::shared_ptr<CatalogRepository> catalogRepo,
    std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn),
      apiRepo_(apiRepo),
      catalogRepo_(catalogRepo),
      cache_(std::make_shared<PartitionsCacheRepository>(hrn, cache)) {
  read::PartitionsResponse cancelledResponse{
      {olp::network::Network::ErrorCode::Cancelled, "Operation cancelled."}};
  multiRequestContext_ =
      std::make_shared<MultiRequestContext<read::PartitionsResponse,
                                           read::PartitionsResponseCallback>>(
          cancelledResponse);
}

CancellationToken PartitionsRepository::GetPartitions(
    const PartitionsRequest& request,
    const PartitionsResponseCallback& callback) {
  // local copy of cache to ensure it will live the duration of the request
  auto apiRepo = apiRepo_;
  auto& cache = *cache_;
  auto cancel_context = std::make_shared<CancellationContext>();

  auto requestKey = request.CreateKey();
  auto cancel_callback = [callback, requestKey]() {
    EDGE_SDK_LOG_INFO_F(kLogTag, "cancelled '%s'", requestKey.c_str());
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetPartitions '%s'", requestKey.c_str());
  MultiRequestContext<read::PartitionsResponse,
                      read::PartitionsResponseCallback>::ExecuteFn executeFn =
      [=, &cache](read::PartitionsResponseCallback callback) {
        auto fetchPartitions = [=, &cache](
                                   const PartitionsRequest& appendedRequest,
                                   const boost::optional<time_t>& expiry) {
          cancel_context->ExecuteOrCancelled(
              [=, &cache]() {
                // Check the cache first
                if (OnlineOnly != request.GetFetchOption()) {
                  auto cachedPartitions = cache.Get(appendedRequest);
                  if (cachedPartitions) {
                    ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
                      EDGE_SDK_LOG_INFO_F(kLogTag,
                                          "cache partitions '%s' found!",
                                          requestKey.c_str());
                      callback(*cachedPartitions);
                    });
                    return CancellationToken();
                  } else if (CacheOnly == request.GetFetchOption()) {
                    ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
                      EDGE_SDK_LOG_INFO_F(kLogTag,
                                          "cache partitions '%s' not found!",
                                          requestKey.c_str());
                      callback(ApiError(ErrorCode::NotFound,
                                        "Cache only resource not found in "
                                        "cache (partitions)."));
                    });
                    return CancellationToken();
                  }
                }

                auto cachePartitionsResponseCallback =
                    [=, &cache](PartitionsResponse response) {
                      if (response.IsSuccessful()) {
                        EDGE_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                                            requestKey.c_str());
                        cache.Put(appendedRequest, response.GetResult(), expiry,
                                  true);
                      } else {
                        if (403 == response.GetError().GetHttpStatusCode()) {
                          EDGE_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                                              requestKey.c_str());
                          cache.Clear(appendedRequest.GetLayerId());
                        }
                      }
                      callback(response);
                    };
                // Network Query
                return apiRepo->getApiClient(
                    "metadata", "v1", [=, &cache](ApiClientResponse response) {
                      if (!response.IsSuccessful()) {
                        EDGE_SDK_LOG_INFO_F(kLogTag,
                                            "getApiClient '%s' unsuccessful",
                                            requestKey.c_str());
                        callback(response.GetError());
                        return;
                      }

                      auto getPartitionsInternal =
                          [=](const boost::optional<int64_t>& version) {
                            cancel_context->ExecuteOrCancelled(
                                [=]() {
                                  EDGE_SDK_LOG_INFO_F(
                                      kLogTag,
                                      "getApiClient '%s' getting partitions",
                                      requestKey.c_str());
                                  return MetadataApi::GetPartitions(
                                      response.GetResult(),
                                      appendedRequest.GetLayerId(), version,
                                      boost::none, boost::none,
                                      appendedRequest.GetBillingTag(),
                                      cachePartitionsResponseCallback);
                                },

                                cancel_callback);
                          };

                      // the catalog version must be set for versioned layers
                      if (appendedRequest.GetVersion()) {
                        GetLayerVersion(
                            cancel_context, response.GetResult(),
                            appendedRequest,
                            [=,
                             &cache](LayerVersionReponse layerVersionResponse) {
                              if (!layerVersionResponse.IsSuccessful()) {
                                if (403 == layerVersionResponse.GetError()
                                               .GetHttpStatusCode()) {
                                  cache.Clear(appendedRequest.GetLayerId());
                                }
                                EDGE_SDK_LOG_INFO_F(kLogTag,
                                                    "GetLayerVersion '%s'",
                                                    requestKey.c_str());
                                callback(layerVersionResponse.GetError());
                                return;
                              }

                              getPartitionsInternal(
                                  layerVersionResponse.GetResult());
                            },
                            cache, apiRepo);
                      } else {
                        EDGE_SDK_LOG_INFO_F(
                            kLogTag, "GetLayerVersion '%s' has no version",
                            requestKey.c_str());
                        getPartitionsInternal(boost::none);
                      }
                    });
              },

              cancel_callback);
        };
        appendPartitionsRequest(request, catalogRepo_, cancel_context,
                                cancel_callback, callback, fetchPartitions);

        return CancellationToken(
            [cancel_context]() { cancel_context->CancelOperation(); });
      };
  return multiRequestContext_->ExecuteOrAssociate(requestKey, executeFn,
                                                  callback);
}

CancellationToken PartitionsRepository::GetPartitionsById(
    const PartitionsRequest& request,
    const std::vector<std::string>& partitions,
    const PartitionsResponseCallback& callback) {
  // local copy of repo to ensure it will live the duration of the request
  auto apiRepo = apiRepo_;
  auto& cache = *cache_;
  auto cancel_context = std::make_shared<CancellationContext>();

  auto requestKey = GetKey(request, partitions);
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetPartitionsById '%s'", requestKey.c_str());

  auto cancel_callback = [callback, requestKey]() {
    EDGE_SDK_LOG_INFO_F(kLogTag, "cancelled '%s'", requestKey.c_str());
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  MultiRequestContext<read::PartitionsResponse,
                      read::PartitionsResponseCallback>::ExecuteFn executeFn =
      [=, &cache](read::PartitionsResponseCallback callback) {
        auto fetchPartitions = [=, &cache](
                                   const PartitionsRequest& appendedRequest,
                                   const boost::optional<time_t>& expiry) {
          cancel_context->ExecuteOrCancelled(
              [=, &cache]() {
                /* Check the cache first */
                if (OnlineOnly != request.GetFetchOption()) {
                  auto cachedPartitions =
                      cache.Get(appendedRequest, partitions);
                  // Only used cache if we have all requested ids.
                  if (cachedPartitions.GetPartitions().size() ==
                      partitions.size()) {
                    ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
                      EDGE_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                                          requestKey.c_str());
                      callback(cachedPartitions);
                    });
                    return CancellationToken();
                  } else if (CacheOnly == request.GetFetchOption()) {
                    ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
                      EDGE_SDK_LOG_INFO_F(kLogTag,
                                          "cache catalog '%s' not found!",
                                          requestKey.c_str());
                      callback(ApiError(ErrorCode::NotFound,
                                        "Cache only resource not found in "
                                        "cache (partition)."));
                    });
                    return CancellationToken();
                  }
                }

                auto cachePartitionsResponseCallback =
                    [=, &cache](PartitionsResponse response) {
                      if (response.IsSuccessful()) {
                        EDGE_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                                            requestKey.c_str());
                        cache.Put(appendedRequest, response.GetResult(),
                                  expiry);
                      } else {
                        if (403 == response.GetError().GetHttpStatusCode()) {
                          EDGE_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                                              requestKey.c_str());
                          // Delete partitions only but not the layer
                          cache.ClearPartitions(appendedRequest, partitions);
                        }
                      }
                      callback(response);
                    };

                return apiRepo->getApiClient(
                    "query", "v1", [=](ApiClientResponse response) {
                      if (!response.IsSuccessful()) {
                        EDGE_SDK_LOG_INFO_F(kLogTag,
                                            "getApiClient '%s' unsuccessful",
                                            requestKey.c_str());
                        callback(response.GetError());
                        return;
                      }

                      cancel_context->ExecuteOrCancelled(
                          [=]() {
                            EDGE_SDK_LOG_INFO_F(
                                kLogTag, "getApiClient '%s' getting catalog",
                                requestKey.c_str());
                            return QueryApi::GetPartitionsbyId(
                                response.GetResult(),
                                appendedRequest.GetLayerId(), partitions,
                                appendedRequest.GetVersion(), boost::none,
                                appendedRequest.GetBillingTag(),
                                cachePartitionsResponseCallback);
                          },
                          cancel_callback);
                    });
              },
              cancel_callback);
        };

        appendPartitionsRequest(request, catalogRepo_, cancel_context,
                                cancel_callback, callback, fetchPartitions);

        return CancellationToken(
            [cancel_context]() { cancel_context->CancelOperation(); });
      };
  return multiRequestContext_->ExecuteOrAssociate(requestKey, executeFn,
                                                  callback);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
