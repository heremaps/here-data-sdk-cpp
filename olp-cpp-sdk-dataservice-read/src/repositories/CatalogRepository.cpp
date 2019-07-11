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

#include "CatalogRepository.h"
#include <iostream>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/logging/Log.h>

#include "ApiRepository.h"
#include "CatalogCacheRepository.h"
#include "generated/api/ConfigApi.h"
#include "generated/api/MetadataApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

using namespace olp::client;

namespace {
constexpr char kCRLogtag[] = "CatalogRepository";

std::string requestKeyFromRequest(const CatalogRequest& request) {
  std::stringstream ss;

  if (request.GetBillingTag()) {
    ss << "$" << request.GetBillingTag().get();
  }

  ss << "^" << request.GetFetchOption();

  return ss.str();
}

std::string requestKeyFromRequest(const CatalogVersionRequest& request) {
  std::stringstream ss;

  ss << "@" << request.GetStartVersion();

  if (request.GetBillingTag()) {
    ss << "$" << request.GetBillingTag().get();
  }

  ss << "^" << request.GetFetchOption();

  return ss.str();
}
}  // namespace

CatalogRepository::CatalogRepository(
    const HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
    std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn),
      apiRepo_(apiRepo),
      cache_(std::make_shared<CatalogCacheRepository>(hrn, cache)) {
  read::CatalogResponse cancelledResponse{
      {olp::network::Network::ErrorCode::Cancelled, "Operation cancelled."}};
  multiRequestContext_ = std::make_shared<MultiRequestContext<
      read::CatalogResponse, read::CatalogResponseCallback>>(cancelledResponse);
}

CancellationToken CatalogRepository::getCatalog(
    const CatalogRequest& request, const CatalogResponseCallback& callback) {
  std::string hrn(hrn_.ToCatalogHRNString());
  LOG_TRACE_F(kCRLogtag, "getCatalog '%s'", hrn.c_str());

  auto cancel_context = std::make_shared<CancellationContext>();
  auto& cache = *cache_;

  auto requestKey = requestKeyFromRequest(request);
  MultiRequestContext<read::CatalogResponse,
                      read::CatalogResponseCallback>::ExecuteFn executeFn =
      [=, &cache](read::CatalogResponseCallback callback) {
        cancel_context->ExecuteOrCancelled(
            [=, &cache]() {
              LOG_TRACE_F(kCRLogtag, "checking catalog '%s' cache",
                          requestKey.c_str());
              // Check the cache
              if (OnlineOnly != request.GetFetchOption()) {
                auto cachedCatalog = cache.Get();
                if (cachedCatalog) {
                  std::thread([=] {
                    LOG_TRACE_F(kCRLogtag, "cache catalog '%s' found!",
                                requestKey.c_str());
                    callback(*cachedCatalog);
                  })
                      .detach();
                  return CancellationToken();
                } else if (CacheOnly == request.GetFetchOption()) {
                  std::thread([=] {
                    LOG_TRACE_F(kCRLogtag, "cache catalog '%s' not found!",
                                requestKey.c_str());
                    callback(ApiError(
                        ErrorCode::NotFound,
                        "Cache only resource not found in cache (catalog)."));
                  })
                      .detach();
                  return CancellationToken();
                }
              }
              // Query Network
              auto cacheCatalogResponseCallback =
                  [=, &cache](CatalogResponse response) {
                    LOG_TRACE_F(kCRLogtag, "network response '%s'",
                                requestKey.c_str());
                    if (response.IsSuccessful()) {
                      LOG_TRACE_F(kCRLogtag, "put '%s' to cache",
                                  requestKey.c_str());
                      cache.Put(response.GetResult());
                    } else {
                      if (403 == response.GetError().GetHttpStatusCode()) {
                        LOG_TRACE_F(kCRLogtag, "clear '%s' cache",
                                    requestKey.c_str());
                        cache.Clear();
                      }
                    }
                    callback(response);
                  };

              return apiRepo_->getApiClient(
                  "config", "v1", [=](ApiClientResponse response) {
                    if (!response.IsSuccessful()) {
                      LOG_TRACE_F(kCRLogtag, "getApiClient '%s' unsuccessful",
                                  requestKey.c_str());
                      callback(response.GetError());
                      return;
                    }

                    cancel_context->ExecuteOrCancelled(
                        [=]() {
                          LOG_TRACE_F(kCRLogtag,
                                      "getApiClient '%s' getting catalog",
                                      requestKey.c_str());
                          return ConfigApi::GetCatalog(
                              response.GetResult(), hrn,
                              request.GetBillingTag(),
                              cacheCatalogResponseCallback);
                        },
                        [=]() {
                          LOG_INFO_F(kCRLogtag, "getApiClient '%s' cancelled",
                                     requestKey.c_str());
                          callback({{ErrorCode::Cancelled,
                                     "Operation cancelled.", true}});
                        });
                  });
            },

            [=]() {
              LOG_INFO_F(kCRLogtag, "Cancelled '%s'", requestKey.c_str());
              callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
            });

        return CancellationToken(
            [cancel_context]() { cancel_context->CancelOperation(); });
      };
  LOG_TRACE_F(kCRLogtag, "ExecuteOrAssociate '%s'", requestKey.c_str());
  return multiRequestContext_->ExecuteOrAssociate(requestKey, executeFn,
                                                  callback);
}

CancellationToken CatalogRepository::getLatestCatalogVersion(
    const CatalogVersionRequest& request,
    const CatalogVersionCallback& callback) {
  auto cancel_context = std::make_shared<CancellationContext>();
  auto& cache = *cache_;
  auto requestKey = requestKeyFromRequest(request);
  cancel_context->ExecuteOrCancelled(
      [=, &cache]() {
        LOG_TRACE_F(kCRLogtag, "checking catalog '%s' cache",
                    requestKey.c_str());
        // Check the cache if cache-only request.
        if (CacheOnly == request.GetFetchOption()) {
          auto cachedVersion = cache.GetVersion();
          if (cachedVersion) {
            std::thread([=] {
              LOG_TRACE_F(kCRLogtag, "cache catalog '%s' found!",
                          requestKey.c_str());
              callback(*cachedVersion);
            })
                .detach();
          } else {
            std::thread([=] {
              LOG_TRACE_F(kCRLogtag, "cache catalog '%s' not found!",
                          requestKey.c_str());
              callback(ApiError(
                  ErrorCode::NotFound,
                  "Cache only resource not found in cache (catalog version)."));
            })
                .detach();
          }
          return CancellationToken();
        }
        // Network Query
        auto cacheVersionResponseCallback =
            [=, &cache](CatalogVersionResponse response) {
              LOG_TRACE_F(kCRLogtag, "network response '%s'",
                          requestKey.c_str());
              if (response.IsSuccessful()) {
                LOG_TRACE_F(kCRLogtag, "put '%s' to cache", requestKey.c_str());
                cache.PutVersion(response.GetResult());
              } else {
                if (403 == response.GetError().GetHttpStatusCode()) {
                  LOG_TRACE_F(kCRLogtag, "clear '%s' cache",
                              requestKey.c_str());
                  cache.Clear();
                }
              }
              callback(response);
            };
        return apiRepo_->getApiClient(
            "metadata", "v1", [=](ApiClientResponse response) {
              if (!response.IsSuccessful()) {
                LOG_TRACE_F(kCRLogtag, "getApiClient '%s' unsuccessful",
                            requestKey.c_str());
                callback(response.GetError());
                return;
              }

              cancel_context->ExecuteOrCancelled(
                  [=]() {
                    LOG_TRACE_F(
                        kCRLogtag,
                        "getApiClient '%s' getting latest catalog version",
                        requestKey.c_str());
                    return MetadataApi::GetLatestCatalogVersion(
                        response.GetResult(), request.GetStartVersion(),
                        request.GetBillingTag(), cacheVersionResponseCallback);
                  },
                  [=]() {
                    LOG_INFO_F(kCRLogtag, "getApiClient '%s' cancelled",
                               requestKey.c_str());
                    callback(
                        {{ErrorCode::Cancelled, "Operation cancelled.", true}});
                  });
            });
      },

      [=]() {
        LOG_TRACE_F(kCRLogtag, "Cancelled '%s'", requestKey.c_str());
        callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
      });

  LOG_TRACE_F(kCRLogtag, "ExecuteOrAssociate '%s'", requestKey.c_str());
  return CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
