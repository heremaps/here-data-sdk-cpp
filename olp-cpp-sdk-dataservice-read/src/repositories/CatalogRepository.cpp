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

#include "ApiClientLookup.h"
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

CatalogResponse CatalogRepository::GetCatalog(
    client::HRN catalog, client::CancellationContext cancellation_context,
    CatalogRequest request, client::OlpClientSettings settings) {
  using namespace client;

  const auto request_key = request.CreateKey();
  const auto fetch_options = request.GetFetchOption();

  OLP_SDK_LOG_TRACE_F(kLogTag, "getCatalog '%s'", request_key.c_str());

  repository::CatalogCacheRepository repository{catalog, settings.cache};

  if (fetch_options != OnlineOnly) {
    auto cached = repository.Get();
    if (cached) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' found!",
                         request_key.c_str());
      return *cached;
    } else if (fetch_options == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                         request_key.c_str());
      return ApiError(ErrorCode::NotFound,
                      "Cache only resource not found in cache (catalog).");
    }
  }

  auto config_api = ApiClientLookup::LookupApi(
      catalog, cancellation_context, "config", "v1", fetch_options, settings);

  if (!config_api.IsSuccessful()) {
    return config_api.GetError();
  }

  const OlpClient& client = config_api.GetResult();
  auto catalog_response =
      ConfigApi::GetCatalog(client, catalog.ToCatalogHRNString(),
                            request.GetBillingTag(), cancellation_context);
  if (catalog_response.IsSuccessful()) {
    repository.Put(catalog_response.GetResult());
  } else {
    const auto& error = catalog_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      repository.Clear();
    }
  }

  return catalog_response;
}

CatalogVersionResponse CatalogRepository::GetLatestVersion(
    client::HRN catalog, client::CancellationContext cancellation_context,
    CatalogVersionRequest request, client::OlpClientSettings settings) {
  using namespace client;

  repository::CatalogCacheRepository repository(catalog, settings.cache);

  auto fetch_option = request.GetFetchOption();
  if (fetch_option != OnlineOnly) {
    auto cached_version = repository.GetVersion();
    if (cached_version) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' found!",
                         request.CreateKey().c_str());
      return cached_version.value();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                         request.CreateKey().c_str());
      return ApiError(
          ErrorCode::NotFound,
          "Cache only resource not found in cache (catalog version).");
    }
  }

  auto metadata_api = ApiClientLookup::LookupApi(
      std::move(catalog), cancellation_context, "metadata", "v1", fetch_option,
      std::move(settings));

  if (!metadata_api.IsSuccessful()) {
    return metadata_api.GetError();
  }

  const client::OlpClient& client = metadata_api.GetResult();

  auto version_response = MetadataApi::GetLatestCatalogVersion(
      client, -1, request.GetBillingTag(), cancellation_context);

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
