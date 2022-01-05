/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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

  repository::CatalogCacheRepository repository(
      catalog_, settings_.cache, settings_.default_cache_expiration);

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
  repository::CatalogCacheRepository repository(
      catalog_, settings_.cache, settings_.default_cache_expiration);

  const auto fetch_option = request.GetFetchOption();
  // in case get version online was never called and version was not found in
  // cache
  CatalogVersionResponse version_response = {
      {client::ErrorCode::NotFound, "Failed to find version."}};

  if (fetch_option != CacheOnly) {
    version_response =
        GetLatestVersionOnline(request.GetBillingTag(), std::move(context));

    if (fetch_option == OnlineOnly) {
      return version_response;
    }

    if (!version_response.IsSuccessful()) {
      const auto& error = version_response.GetError();
      if (error.GetHttpStatusCode() == http::HttpStatusCode::FORBIDDEN) {
        OLP_SDK_LOG_WARNING_F(
            kLogTag,
            "Latest version request ended with 403 HTTP code, hrn='%s'",
            catalog_.ToCatalogHRNString().c_str());
        repository.Clear();
        return version_response;
      }
    }
  }

  auto cached_version = repository.GetVersion();

  // Using `GetStartVersion` to set up new latest version for CacheOnly
  // requests in case of absence of previous latest version or it less than the
  // new user set version
  if (fetch_option == CacheOnly) {
    constexpr auto kDefaultStartVersion = 0;

    auto user_set_version = request.GetStartVersion();
    if (user_set_version >= kDefaultStartVersion) {
      if (!cached_version || user_set_version > cached_version->GetVersion()) {
        model::VersionResponse new_response;
        new_response.SetVersion(user_set_version);
        version_response = new_response;
      }
    }
  }

  if (version_response.IsSuccessful()) {
    const auto new_version = version_response.GetResult().GetVersion();
    // Write or update the version in cache, updating happens only when the new
    // version is greater than cached.
    if (!cached_version || (*cached_version).GetVersion() < new_version) {
      repository.PutVersion(version_response.GetResult());
      if (fetch_option == CacheOnly) {
        OLP_SDK_LOG_DEBUG_F(
            kLogTag, "Latest user set version, hrn='%s', version=%" PRId64,
            catalog_.ToCatalogHRNString().c_str(), new_version);
      } else {
        OLP_SDK_LOG_DEBUG_F(kLogTag,
                            "Latest online version, hrn='%s', version=%" PRId64,
                            catalog_.ToCatalogHRNString().c_str(), new_version);
      }
    }
    return version_response;
  }

  if (cached_version) {
    OLP_SDK_LOG_DEBUG_F(
        kLogTag, "Latest cached version, hrn='%s', version=%" PRId64,
        catalog_.ToCatalogHRNString().c_str(), (*cached_version).GetVersion());
    version_response = *std::move(cached_version);
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

CatalogVersionResponse CatalogRepository::GetLatestVersionOnline(
    const boost::optional<std::string>& billing_tag,
    client::CancellationContext context) {
  auto metadata_api = lookup_client_.LookupApi(
      "metadata", "v1", client::OnlineIfNotFound, context);

  if (!metadata_api.IsSuccessful()) {
    return metadata_api.GetError();
  }

  const client::OlpClient& metadata_client = metadata_api.GetResult();

  return MetadataApi::GetLatestCatalogVersion(metadata_client, -1, billing_tag,
                                              context);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
