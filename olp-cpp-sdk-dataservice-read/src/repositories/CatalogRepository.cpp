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
#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>

#include "ApiRepository.h"
#include "CatalogCacheRepository.h"
#include "ExecuteOrSchedule.inl"
#include "generated/api/ConfigApi.h"
#include "generated/api/MetadataApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "CatalogRepository";

}  // namespace

CatalogRepository::CatalogRepository(
    const client::HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
    std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn),
      apiRepo_(apiRepo),
      cache_(std::make_shared<CatalogCacheRepository>(hrn, cache)) {
  CatalogResponse cancelledResponse{
      {static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
       "Operation cancelled."}};
  multiRequestContext_ =
      std::make_shared<MultiRequestContext<CatalogResponse>>(cancelledResponse);
}

client::CancellationToken CatalogRepository::getCatalog(
    const CatalogRequest& request, const CatalogResponseCallback& callback) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto& cache = cache_;

  auto requestKey = request.CreateKey();
  OLP_SDK_LOG_TRACE_F(kLogTag, "getCatalog '%s'", requestKey.c_str());

  auto executeFn = [=](CatalogResponseCallback callback) {
    cancel_context->ExecuteOrCancelled(
        [=]() {
          OLP_SDK_LOG_INFO_F(kLogTag, "checking catalog '%s' cache",
                             requestKey.c_str());
          // Check the cache
          if (OnlineOnly != request.GetFetchOption()) {
            auto cachedCatalog = cache->Get();
            if (cachedCatalog) {
              ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
                OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' found!",
                                   requestKey.c_str());
                callback(*cachedCatalog);
              });
              return client::CancellationToken();
            } else if (CacheOnly == request.GetFetchOption()) {
              ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
                OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                                   requestKey.c_str());
                callback(client::ApiError(
                    client::ErrorCode::NotFound,
                    "Cache only resource not found in cache (catalog)."));
              });
              return client::CancellationToken();
            }
          }
          // Query Network
          auto cacheCatalogResponseCallback = [=](CatalogResponse response) {
            OLP_SDK_LOG_INFO_F(kLogTag, "network response '%s'",
                               requestKey.c_str());
            if (response.IsSuccessful()) {
              OLP_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                                 requestKey.c_str());
              cache->Put(response.GetResult());
            } else {
              if (403 == response.GetError().GetHttpStatusCode()) {
                OLP_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                                   requestKey.c_str());
                cache->Clear();
              }
            }
            callback(response);
          };

          return apiRepo_->getApiClient(
              "config", "v1", [=](ApiClientResponse response) {
                if (!response.IsSuccessful()) {
                  OLP_SDK_LOG_INFO_F(kLogTag, "getApiClient '%s' unsuccessful",
                                     requestKey.c_str());
                  callback(response.GetError());
                  return;
                }

                cancel_context->ExecuteOrCancelled(
                    [=]() {
                      OLP_SDK_LOG_INFO_F(kLogTag,
                                         "getApiClient '%s' getting catalog",
                                         requestKey.c_str());
                      return ConfigApi::GetCatalog(
                          response.GetResult(), hrn, request.GetBillingTag(),
                          cacheCatalogResponseCallback);
                    },
                    [=]() {
                      OLP_SDK_LOG_INFO_F(kLogTag, "getApiClient '%s' cancelled",
                                         requestKey.c_str());
                      callback({{client::ErrorCode::Cancelled,
                                 "Operation cancelled.", true}});
                    });
              });
        },

        [=]() {
          OLP_SDK_LOG_INFO_F(kLogTag, "Cancelled '%s'", requestKey.c_str());
          callback(
              {{client::ErrorCode::Cancelled, "Operation cancelled.", true}});
        });

    return client::CancellationToken(
        [cancel_context]() { cancel_context->CancelOperation(); });
  };
  OLP_SDK_LOG_INFO_F(kLogTag, "ExecuteOrAssociate '%s'", requestKey.c_str());
  return multiRequestContext_->ExecuteOrAssociate(requestKey, executeFn,
                                                  callback);
}

client::CancellationToken CatalogRepository::getLatestCatalogVersion(
    const CatalogVersionRequest& request,
    const CatalogVersionCallback& callback) {
  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto& cache = cache_;

  auto requestKey = request.CreateKey();
  OLP_SDK_LOG_TRACE_F(kLogTag, "getCatalogVersion '%s'", requestKey.c_str());

  cancel_context->ExecuteOrCancelled(
      [=]() {
        OLP_SDK_LOG_INFO_F(kLogTag, "checking catalog '%s' cache",
                           requestKey.c_str());
        // Check the cache if cache-only request.
        if (CacheOnly == request.GetFetchOption()) {
          auto cachedVersion = cache->GetVersion();
          if (cachedVersion) {
            ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
              OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' found!",
                                 requestKey.c_str());
              callback(*cachedVersion);
            });
          } else {
            ExecuteOrSchedule(apiRepo_->GetOlpClientSettings(), [=] {
              OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                                 requestKey.c_str());
              callback(client::ApiError(
                  client::ErrorCode::NotFound,
                  "Cache only resource not found in cache (catalog version)."));
            });
          }
          return client::CancellationToken();
        }
        // Network Query
        auto cacheVersionResponseCallback =
            [=](CatalogVersionResponse response) {
              OLP_SDK_LOG_INFO_F(kLogTag, "network response '%s'",
                                 requestKey.c_str());
              if (response.IsSuccessful()) {
                OLP_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache",
                                   requestKey.c_str());
                cache->PutVersion(response.GetResult());
              } else {
                if (403 == response.GetError().GetHttpStatusCode()) {
                  OLP_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache",
                                     requestKey.c_str());
                  cache->Clear();
                }
              }
              callback(response);
            };
        return apiRepo_->getApiClient(
            "metadata", "v1", [=](ApiClientResponse response) {
              if (!response.IsSuccessful()) {
                OLP_SDK_LOG_INFO_F(kLogTag, "getApiClient '%s' unsuccessful",
                                   requestKey.c_str());
                callback(response.GetError());
                return;
              }

              cancel_context->ExecuteOrCancelled(
                  [=]() {
                    OLP_SDK_LOG_INFO_F(
                        kLogTag,
                        "getApiClient '%s' getting latest catalog version",
                        requestKey.c_str());
                    return MetadataApi::GetLatestCatalogVersion(
                        response.GetResult(), request.GetStartVersion(),
                        request.GetBillingTag(), cacheVersionResponseCallback);
                  },
                  [=]() {
                    OLP_SDK_LOG_INFO_F(kLogTag, "getApiClient '%s' cancelled",
                                       requestKey.c_str());
                    callback({{client::ErrorCode::Cancelled,
                               "Operation cancelled.", true}});
                  });
            });
      },

      [=]() {
        OLP_SDK_LOG_INFO_F(kLogTag, "Cancelled '%s'", requestKey.c_str());
        callback(
            {{client::ErrorCode::Cancelled, "Operation cancelled.", true}});
      });

  OLP_SDK_LOG_INFO_F(kLogTag, "ExecuteOrAssociate '%s'", requestKey.c_str());
  return client::CancellationToken(
      [cancel_context]() { cancel_context->CancelOperation(); });
}

MetadataApi::CatalogVersionResponse CatalogRepository::GetLatestVersion(
    const client::HRN& catalog,
    client::CancellationContext cancellation_context,
    const DataRequest& data_request, client::OlpClientSettings settings) {
  using namespace client;

  repository::CatalogCacheRepository repository(catalog, settings.cache);

  auto fetch_option = data_request.GetFetchOption();
  if (fetch_option != OnlineOnly) {
    auto cached_version = repository.GetVersion();
    if (cached_version) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' found!",
                         data_request.CreateKey().c_str());
      return cached_version.value();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                         data_request.CreateKey().c_str());
      return ApiError(
          ErrorCode::NotFound,
          "Cache only resource not found in cache (catalog version).");
    }
  }

  const auto timeout = settings.retry_settings.timeout;

  auto metadata_api =
      ApiClientLookup::LookupApi(catalog, cancellation_context, "metadata",
                                 "v1", fetch_option, std::move(settings));

  if (!metadata_api.IsSuccessful()) {
    return metadata_api.GetError();
  }

  const client::OlpClient& client = metadata_api.GetResult();

  Condition condition{};

  // when the network operation took too much time we cancel it and exit
  // execution, to make sure that network callback will not access dangling
  // references we protect them with atomic bool flag.
  auto interest_flag = std::make_shared<std::atomic_bool>(true);

  MetadataApi::CatalogVersionResponse version_response;
  cancellation_context.ExecuteOrCancelled(
      [&]() {
        auto token = MetadataApi::GetLatestCatalogVersion(
            client, -1, data_request.GetBillingTag(),
            [&, interest_flag](MetadataApi::CatalogVersionResponse response) {
              if (interest_flag->exchange(false)) {
                version_response = std::move(response);
                condition.Notify();
              }
            });

        return client::CancellationToken([&, interest_flag, token]() {
          if (interest_flag->exchange(false)) {
            token.cancel();
            condition.Notify();
          }
        });
      },
      [&condition]() { condition.Notify(); });

  if (!condition.Wait(std::chrono::seconds{timeout})) {
    cancellation_context.CancelOperation();
    return ApiError(ErrorCode::RequestTimeout, "Network request timed out.");
  }
  interest_flag->store(false);

  if (cancellation_context.IsCancelled()) {
    return ApiError(ErrorCode::Cancelled, "Operation cancelled.");
  }

  if (version_response.IsSuccessful()) {
    repository.PutVersion(version_response.GetResult());
  } else {
    const auto& error = version_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      repository.Clear();
    }
  }

  return version_response;
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
