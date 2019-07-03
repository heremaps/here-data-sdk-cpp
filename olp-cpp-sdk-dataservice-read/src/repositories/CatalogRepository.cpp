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
std::string requestKeyFromRequest(const CatalogRequest& request) {
  std::stringstream ss;

  if (request.GetBillingTag()) {
    ss << "$" << request.GetBillingTag().get();
  }

  ss << "^" << request.GetFetchOption();

  return ss.str();
}
}  // namespace

#define CR_LOGTAG "CatalogRepository"

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

  auto cancel_context = std::make_shared<CancellationContext>();
  auto& cache = *cache_;

  auto requestKey = requestKeyFromRequest(request);
  MultiRequestContext<read::CatalogResponse,
                      read::CatalogResponseCallback>::ExecuteFn executeFn =
      [=, &cache](read::CatalogResponseCallback callback) {
        cancel_context->ExecuteOrCancelled(
            [=, &cache]() {
              LOG_TRACE_F(CR_LOGTAG, "cancel_context->Execute");
              // Check the cache
              if (OnlineOnly != request.GetFetchOption()) {
                auto cachedCatalog = cache.Get();
                if (cachedCatalog) {
                  std::thread([=] {
                    LOG_TRACE_F(CR_LOGTAG, "CB CACHED");
                    callback(*cachedCatalog);
                  })
                      .detach();
                  return CancellationToken();
                } else if (CacheOnly == request.GetFetchOption()) {
                  std::thread([=] {
                    LOG_TRACE_F(CR_LOGTAG, "CB ERROR");
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
                    LOG_TRACE_F(CR_LOGTAG, "cache repo context");
                    if (response.IsSuccessful()) {
                      cache.Put(response.GetResult());
                    } else {
                      if (403 == response.GetError().GetHttpStatusCode()) {
                        cache.Clear();
                      }
                    }
                    callback(response);
                  };

              return apiRepo_->getApiClient(
                  "config", "v1", [=](ApiClientResponse response) {
                    LOG_TRACE_F(CR_LOGTAG, "getApiClient");
                    if (!response.IsSuccessful()) {
                      callback(response.GetError());
                      return;
                    }

                    cancel_context->ExecuteOrCancelled(
                        [=]() {
                          return ConfigApi::GetCatalog(
                              response.GetResult(), hrn,
                              request.GetBillingTag(),
                              cacheCatalogResponseCallback);
                        },
                        [=]() {
                          callback({{ErrorCode::Cancelled,
                                     "Operation cancelled.", true}});
                        });
                  });
            },

            [=]() {
              LOG_TRACE_F(CR_LOGTAG, "Cancelled");
              callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
            });

        return CancellationToken(
            [cancel_context]() { cancel_context->CancelOperation(); });
      };
  LOG_TRACE_F(CR_LOGTAG, "b4 ExecuteOrAssociate");
  return multiRequestContext_->ExecuteOrAssociate(requestKey, executeFn,
                                                  callback);
}

CancellationToken CatalogRepository::getLatestCatalogVersion(
    const CatalogVersionRequest& request,
    const CatalogVersionCallback& callback) {
  auto cancel_context = std::make_shared<CancellationContext>();
  auto& cache = *cache_;
  cancel_context->ExecuteOrCancelled(
      [=, &cache]() {
        // Check the cache if cache-only request.
        if (CacheOnly == request.GetFetchOption()) {
          auto cachedVersion = cache.GetVersion();
          if (cachedVersion) {
            std::thread([=] { callback(*cachedVersion); }).detach();
          } else {
            std::thread([=] {
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
              if (response.IsSuccessful()) {
                cache.PutVersion(response.GetResult());
              } else {
                if (403 == response.GetError().GetHttpStatusCode()) {
                  cache.Clear();
                }
              }
              callback(response);
            };
        return apiRepo_->getApiClient(
            "metadata", "v1", [=](ApiClientResponse response) {
              if (!response.IsSuccessful()) {
                callback(response.GetError());
                return;
              }

              cancel_context->ExecuteOrCancelled(
                  [=]() {
                    return MetadataApi::GetLatestCatalogVersion(
                        response.GetResult(), request.GetStartVersion(),
                        request.GetBillingTag(), cacheVersionResponseCallback);
                  },
                  [=]() {
                    callback(
                        {{ErrorCode::Cancelled, "Operation cancelled.", true}});
                  });
            });
      },

      [=]() {
        callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
      });

  return CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
