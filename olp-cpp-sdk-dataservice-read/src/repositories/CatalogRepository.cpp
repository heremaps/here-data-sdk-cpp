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

#include "CatalogRepository.h"

#include <iostream>
#include <utility>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/Condition.h>
#include <olp/core/logging/Log.h>

#include "CatalogCacheRepository.h"
#include "ExecuteOrSchedule.inl"
#include "generated/api/ConfigApi.h"
#include "generated/api/MetadataApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/VersionsRequest.h"

namespace {
constexpr auto kLogTag = "CatalogRepository";
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

CatalogRepository::CatalogRepository(client::HRN catalog,
                                     client::OlpClientSettings settings,
                                     client::ApiLookupClient client)
    : catalog_(std::move(catalog)),
      settings_(std::move(settings)),
      lookup_client_(std::move(client)) {}

CatalogResponse CatalogRepository::GetCatalog(
    const CatalogRequest& request, client::CancellationContext context) {
  const auto request_key = request.CreateKey();
  const auto fetch_options = request.GetFetchOption();
  const auto catalog_str = catalog_.ToCatalogHRNString();

  repository::CatalogCacheRepository repository{
      catalog_, settings_.cache, settings_.default_cache_expiration};

  if (fetch_options != OnlineOnly && fetch_options != CacheWithUpdate) {
    auto cached = repository.Get();
    if (cached) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "GetCatalog found in cache, hrn='%s', key='%s'",
                          catalog_str.c_str(), request_key.c_str());

      return *cached;
    } else if (fetch_options == CacheOnly) {
      OLP_SDK_LOG_INFO_F(kLogTag,
                         "GetCatalog not found in cache, hrn='%s', key='%s'",
                         catalog_str.c_str(), request_key.c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  auto config_api = lookup_client_.LookupApi(
      "config", "v1", static_cast<client::FetchOptions>(fetch_options),
      context);

  if (!config_api.IsSuccessful()) {
    return config_api.GetError();
  }

  const client::OlpClient& config_client = config_api.GetResult();
  auto catalog_response = ConfigApi::GetCatalog(
      config_client, catalog_str, request.GetBillingTag(), context);

  if (catalog_response.IsSuccessful() && fetch_options != OnlineOnly) {
    repository.Put(catalog_response.GetResult());
  }
  if (!catalog_response.IsSuccessful()) {
    const auto& error = catalog_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "GetCatalog 403 received, remove from cache, "
                            "hrn='%s', key='%s'",
                            catalog_str.c_str(), request_key.c_str());
      repository.Clear();
    }
  }

  return catalog_response;
}

CatalogVersionResponse CatalogRepository::GetLatestVersion(
    const CatalogVersionRequest& request, client::CancellationContext context) {
  repository::CatalogCacheRepository repository(catalog_, settings_.cache);

  auto fetch_option = request.GetFetchOption();
  if (fetch_option != OnlineOnly && fetch_option != CacheWithUpdate) {
    auto cached_version = repository.GetVersion();
    if (cached_version) {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag, "GetLatestVersion found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), request.CreateKey().c_str());
      return cached_version.value();
    } else if (fetch_option == CacheOnly) {
      OLP_SDK_LOG_INFO_F(
          kLogTag, "GetLatestVersion not found in cache, hrn='%s', key='%s'",
          catalog_.ToCatalogHRNString().c_str(), request.CreateKey().c_str());
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  auto metadata_api = lookup_client_.LookupApi(
      "metadata", "v1", static_cast<client::FetchOptions>(fetch_option),
      context);

  if (!metadata_api.IsSuccessful()) {
    return metadata_api.GetError();
  }

  const client::OlpClient& metadata_client = metadata_api.GetResult();

  auto version_response = MetadataApi::GetLatestCatalogVersion(
      metadata_client, -1, request.GetBillingTag(), context);

  if (version_response.IsSuccessful() && fetch_option != OnlineOnly) {
    repository.PutVersion(version_response.GetResult());
  }

  if (!version_response.IsSuccessful()) {
    const auto& error = version_response.GetError();
    if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "GetLatestVersion 403 received, remove from cache, "
                            "hrn='%s', key='%s'",
                            catalog_.ToCatalogHRNString().c_str(),
                            request.CreateKey().c_str());
      repository.Clear();
    }
  }

  return version_response;
}

VersionsResponse CatalogRepository::GetVersionsList(
    const VersionsRequest& request, client::CancellationContext context) {
  auto metadata_api =
      lookup_client_.LookupApi("metadata", "v1", client::OnlineOnly, context);

  if (!metadata_api.IsSuccessful()) {
    return metadata_api.GetError();
  }

  const client::OlpClient& metadata_client = metadata_api.GetResult();

  return MetadataApi::ListVersions(metadata_client, request.GetStartVersion(),
                                   request.GetEndVersion(),
                                   request.GetBillingTag(), context);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
