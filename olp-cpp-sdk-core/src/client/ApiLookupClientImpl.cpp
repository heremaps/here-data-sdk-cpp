/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "ApiLookupClientImpl.h"

#include <olp/core/logging/Log.h>
#include "client/api/PlatformApi.h"
#include "client/api/ResourcesApi.h"
#include "repository/ApiCacheRepository.h"

namespace olp {
namespace client {

namespace {
constexpr auto kLogTag = "ApiLookupClientImpl";

std::string FindApi(const Apis& apis, const std::string& service,
                    const std::string& version) {
  auto it = std::find_if(apis.begin(), apis.end(), [&](const Api& obj) -> bool {
    return (obj.GetApi() == service && obj.GetVersion() == version);
  });
  if (it == apis.end()) {
    return {};
  }

  return it->GetBaseUrl();
}

olp::client::OlpClient GetStaticUrl(
    const olp::client::HRN& catalog,
    const olp::client::OlpClientSettings& settings) {
  const auto& provider = settings.api_lookup_settings.catalog_endpoint_provider;
  if (provider) {
    auto url = provider(catalog);
    if (!url.empty()) {
      url += "/catalogs/" + catalog.ToCatalogHRNString();
      OlpClient result_client;
      result_client.SetBaseUrl(url);
      result_client.SetSettings(settings);
      return result_client;
    }
  }

  return {};
}

}  // namespace

ApiLookupClientImpl::ApiLookupClientImpl(const HRN& catalog,
                                         const OlpClientSettings& settings)
    : catalog_(catalog), settings_(settings) {
  auto provider = settings_.api_lookup_settings.lookup_endpoint_provider;
  const auto& base_url = provider(catalog_.GetPartition());
  lookup_client_.SetBaseUrl(base_url);
  lookup_client_.SetSettings(settings_);
}

ApiLookupClient::LookupApiResponse ApiLookupClientImpl::LookupApi(
    const std::string& service, const std::string& service_version,
    FetchOptions options, CancellationContext context) {
  auto result_client = GetStaticUrl(catalog_, settings_);
  if (!result_client.GetBaseUrl().empty()) {
    return result_client;
  }

  repository::ApiCacheRepository repository(catalog_, settings_.cache);
  const auto hrn = catalog_.ToCatalogHRNString();

  if (options != OnlineOnly && options != CacheWithUpdate) {
    auto url = repository.Get(service, service_version);
    if (url) {
      OLP_SDK_LOG_DEBUG_F(kLogTag, "LookupApi(%s/%s) found in cache, hrn='%s'",
                          service.c_str(), service_version.c_str(),
                          hrn.c_str());
      result_client.SetBaseUrl(*url);
      result_client.SetSettings(settings_);
      return result_client;
    } else if (options == CacheOnly) {
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "LookupApi(%s/%s) cache miss, requesting, hrn='%s'",
                      service.c_str(), service_version.c_str(), hrn.c_str());

  PlatformApi::ApisResponse api_response;
  if (service == "config") {
    api_response = PlatformApi::GetApis(lookup_client_, context);
  } else {
    api_response = ResourcesApi::GetApis(lookup_client_, hrn, context);
  }

  if (!api_response.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "LookupApi(%s/%s) unsuccessful, hrn='%s', error='%s'",
                          service.c_str(), service_version.c_str(), hrn.c_str(),
                          api_response.GetError().GetMessage().c_str());
    return api_response.GetError();
  }

  const auto& api_result = api_response.GetResult();
  if (options != OnlineOnly && options != CacheWithUpdate) {
    for (const auto& service_api : api_result.first) {
      repository.Put(service_api.GetApi(), service_api.GetVersion(),
                     service_api.GetBaseUrl(), api_result.second);
    }
  }

  auto url = FindApi(api_result.first, service, service_version);
  if (url.empty()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag, "LookupApi(%s/%s) service not found, hrn='%s'",
        service.c_str(), service_version.c_str(), hrn.c_str());

    return {{client::ErrorCode::ServiceUnavailable,
             "Service/Version not available for given HRN"}};
  }

  OLP_SDK_LOG_DEBUG_F(
      kLogTag, "LookupApi(%s/%s) found, hrn='%s', service_url='%s'",
      service.c_str(), service_version.c_str(), hrn.c_str(), url.c_str());

  result_client.SetBaseUrl(url);
  result_client.SetSettings(settings_);
  return result_client;
}

CancellationToken ApiLookupClientImpl::LookupApi(
    const std::string& service, const std::string& service_version,
    FetchOptions options, ApiLookupClient::LookupApiCallback callback) {
  auto result_client = GetStaticUrl(catalog_, settings_);
  if (!result_client.GetBaseUrl().empty()) {
    callback(result_client);
    return CancellationToken();
  }

  repository::ApiCacheRepository repository(catalog_, settings_.cache);
  const auto hrn = catalog_.ToCatalogHRNString();

  if (options != OnlineOnly && options != CacheWithUpdate) {
    auto url = repository.Get(service, service_version);
    if (url) {
      OLP_SDK_LOG_DEBUG_F(kLogTag, "LookupApi(%s/%s) found in cache, hrn='%s'",
                          service.c_str(), service_version.c_str(),
                          hrn.c_str());
      result_client.SetBaseUrl(*url);
      result_client.SetSettings(settings_);
      callback(result_client);
      return CancellationToken();
    } else if (options == CacheOnly) {
      ApiLookupClient::LookupApiResponse response{
          {client::ErrorCode::NotFound,
           "CacheOnly: resource not found in cache"}};
      callback(response);
      return CancellationToken();
    }
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "LookupApi(%s/%s) cache miss, requesting, hrn='%s'",
                      service.c_str(), service_version.c_str(), hrn.c_str());

  PlatformApi::ApisCallback lookup_callback =
      [=](PlatformApi::ApisResponse response) mutable -> void {
    if (!response.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "LookupApi(%s/%s) unsuccessful, hrn='%s', error='%s'",
          service.c_str(), service_version.c_str(), hrn.c_str(),
          response.GetError().GetMessage().c_str());
      callback(response.GetError());
      return;
    }

    const auto& api_result = response.GetResult();
    if (options != OnlineOnly && options != CacheWithUpdate) {
      for (const auto& service_api : api_result.first) {
        repository.Put(service_api.GetApi(), service_api.GetVersion(),
                       service_api.GetBaseUrl(), api_result.second);
      }
    }

    const auto url = FindApi(api_result.first, service, service_version);
    if (url.empty()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "LookupApi(%s/%s) service not found, hrn='%s'",
          service.c_str(), service_version.c_str(), hrn.c_str());

      callback({{client::ErrorCode::ServiceUnavailable,
                 "Service/Version not available for given HRN"}});
      return;
    }

    OLP_SDK_LOG_DEBUG_F(
        kLogTag, "LookupApi(%s/%s) found, hrn='%s', service_url='%s'",
        service.c_str(), service_version.c_str(), hrn.c_str(), url.c_str());

    OlpClient result_client;
    result_client.SetBaseUrl(url);
    result_client.SetSettings(settings_);
    callback(result_client);
  };

  if (service == "config") {
    return PlatformApi::GetApis(lookup_client_, lookup_callback);
  }
  return ResourcesApi::GetApis(lookup_client_, hrn, lookup_callback);
}

}  // namespace client
}  // namespace olp
