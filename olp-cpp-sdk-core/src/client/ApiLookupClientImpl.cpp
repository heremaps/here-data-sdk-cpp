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

#include <olp/core/client/HRN.h>
#include <olp/core/logging/Log.h>
#include "client/api/PlatformApi.h"
#include "client/api/ResourcesApi.h"
#include "repository/ApiCacheRepository.h"

namespace olp {
namespace client {

namespace {
constexpr auto kLogTag = "ApiLookupClientImpl";
constexpr time_t kLookupApiDefaultExpiryTime = 3600;
constexpr time_t kLookupApiShortExpiryTime = 300;

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

OlpClient CreateClient(const std::string& base_url,
                       const OlpClientSettings& settings) {
  OlpClient client;
  client.SetBaseUrl(base_url);
  client.SetSettings(settings);
  return client;
}

olp::client::OlpClient GetStaticUrl(
    const olp::client::HRN& catalog,
    const olp::client::OlpClientSettings& settings) {
  const auto& provider = settings.api_lookup_settings.catalog_endpoint_provider;
  if (provider) {
    auto url = provider(catalog);
    if (!url.empty()) {
      url += "/catalogs/" + catalog.ToCatalogHRNString();
      return CreateClient(url, settings);
    }
  }

  return {};
}

ApiLookupClient::LookupApiResponse NotFoundInCacheError() {
  return ApiError(client::ErrorCode::NotFound,
                  "CacheOnly: resource not found in cache");
}

ApiLookupClient::LookupApiResponse ServiceNotAvailable() {
  return ApiError(client::ErrorCode::ServiceUnavailable,
                  "Service/Version not available for given HRN");
}

std::string ClientCacheKey(const std::string& service,
                           const std::string& service_version) {
  return service + service_version;
}
}  // namespace

ApiLookupClientImpl::ApiLookupClientImpl(const HRN& catalog,
                                         const OlpClientSettings& settings)
    : catalog_(catalog),
      catalog_string_(catalog_.ToString()),
      settings_(settings) {
  auto provider = settings_.api_lookup_settings.lookup_endpoint_provider;
  const auto& base_url = provider(catalog_.GetPartition());
  lookup_client_ = CreateClient(base_url, settings_);
}

ApiLookupClient::LookupApiResponse ApiLookupClientImpl::LookupApi(
    const std::string& service, const std::string& service_version,
    FetchOptions options, CancellationContext context) {
  auto result_client = GetStaticUrl(catalog_, settings_);
  if (!result_client.GetBaseUrl().empty()) {
    return result_client;
  }

  if (options != OnlineOnly && options != CacheWithUpdate) {
    auto client = GetCachedClient(service, service_version);
    if (client) {
      return *client;
    } else if (options == CacheOnly) {
      return NotFoundInCacheError();
    }
  }

  PlatformApi::ApisResponse api_response;
  if (service == "config") {
    api_response = PlatformApi::GetApis(lookup_client_, context);
  } else {
    api_response =
        ResourcesApi::GetApis(lookup_client_, catalog_string_, context);
  }

  if (!api_response.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag, "LookupApi(%s/%s) unsuccessful, hrn='%s', error='%s'",
        service.c_str(), service_version.c_str(), catalog_string_.c_str(),
        api_response.GetError().GetMessage().c_str());
    return api_response.GetError();
  }

  const auto& api_result = api_response.GetResult();
  if (options != OnlineOnly && options != CacheWithUpdate) {
    PutToDiskCache(api_result);
  }

  auto url = FindApi(api_result.first, service, service_version);
  if (url.empty()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag, "LookupApi(%s/%s) service not found, hrn='%s'",
        service.c_str(), service_version.c_str(), catalog_string_.c_str());

    return ServiceNotAvailable();
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "LookupApi(%s/%s) found, hrn='%s', service_url='%s'",
                      service.c_str(), service_version.c_str(),
                      catalog_string_.c_str(), url.c_str());

  const auto expiration = api_result.second;

  return CreateAndCacheClient(url, ClientCacheKey(service, service_version),
                              expiration);
}

CancellationToken ApiLookupClientImpl::LookupApi(
    const std::string& service, const std::string& service_version,
    FetchOptions options, ApiLookupClient::LookupApiCallback callback) {
  auto result_client = GetStaticUrl(catalog_, settings_);
  if (!result_client.GetBaseUrl().empty()) {
    callback(result_client);
    return CancellationToken();
  }

  if (options != OnlineOnly && options != CacheWithUpdate) {
    auto client = GetCachedClient(service, service_version);
    if (client) {
      callback(*client);
      return CancellationToken();
    } else if (options == CacheOnly) {
      callback(NotFoundInCacheError());
      return CancellationToken();
    }
  }

  PlatformApi::ApisCallback lookup_callback =
      [=](PlatformApi::ApisResponse response) mutable -> void {
    if (!response.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "LookupApi(%s/%s) unsuccessful, hrn='%s', error='%s'",
          service.c_str(), service_version.c_str(), catalog_string_.c_str(),
          response.GetError().GetMessage().c_str());
      callback(response.GetError());
      return;
    }

    const auto& api_result = response.GetResult();
    if (options != OnlineOnly && options != CacheWithUpdate) {
      PutToDiskCache(api_result);
    }

    const auto url = FindApi(api_result.first, service, service_version);
    if (url.empty()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "LookupApi(%s/%s) service not found, hrn='%s'",
          service.c_str(), service_version.c_str(), catalog_string_.c_str());

      callback(ServiceNotAvailable());
      return;
    }

    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "LookupApi(%s/%s) found, hrn='%s', service_url='%s'",
                        service.c_str(), service_version.c_str(),
                        catalog_string_.c_str(), url.c_str());

    callback(CreateAndCacheClient(url, ClientCacheKey(service, service_version),
                                  api_result.second));
  };

  if (service == "config") {
    return PlatformApi::GetApis(lookup_client_, lookup_callback);
  }
  return ResourcesApi::GetApis(lookup_client_, catalog_string_,
                               lookup_callback);
}

OlpClient ApiLookupClientImpl::CreateAndCacheClient(
    const std::string& base_url, const std::string& cache_key,
    boost::optional<time_t> expiration) {
  std::lock_guard<std::mutex> lock(cached_clients_mutex_);
  ClientWithExpiration& client_with_expiration = cached_clients_[cache_key];

  const auto current_base_url = client_with_expiration.client.GetBaseUrl();
  if (current_base_url.empty()) {
    client_with_expiration.client.SetSettings(settings_);
  }
  if (current_base_url != base_url) {
    client_with_expiration.client.SetBaseUrl(base_url);
  }

  client_with_expiration.expire_at =
      std::chrono::steady_clock::now() +
      std::chrono::seconds(expiration.value_or(kLookupApiDefaultExpiryTime));
  return client_with_expiration.client;
}

boost::optional<OlpClient> ApiLookupClientImpl::GetCachedClient(
    const std::string& service, const std::string& service_version) {
  const std::string key = ClientCacheKey(service, service_version);

  {
    std::lock_guard<std::mutex> lock(cached_clients_mutex_);
    const auto client_it = cached_clients_.find(key);
    if (client_it != cached_clients_.end()) {
      const ClientWithExpiration& client_with_expiration = client_it->second;
      if (client_with_expiration.expire_at > std::chrono::steady_clock::now()) {
        OLP_SDK_LOG_DEBUG_F(
            kLogTag, "LookupApi(%s/%s) found in client cache, hrn='%s'",
            service.c_str(), service_version.c_str(), catalog_string_.c_str());
        return client_with_expiration.client;
      }
    }
  }

  repository::ApiCacheRepository cache_repository_(catalog_, settings_.cache);
  const auto base_url = cache_repository_.Get(service, service_version);
  if (base_url) {
    OLP_SDK_LOG_DEBUG_F(
        kLogTag, "LookupApi(%s/%s) found in disk cache, hrn='%s'",
        service.c_str(), service_version.c_str(), catalog_string_.c_str());
  } else {
    OLP_SDK_LOG_DEBUG_F(
        kLogTag, "LookupApi(%s/%s) cache miss in disk cache, hrn='%s'",
        service.c_str(), service_version.c_str(), catalog_string_.c_str());
    return boost::none;
  }

  // When the service url is retrieved from disk cache we assume it is valid for
  // five minutes, after five minutes we repeat. This is because we cannot
  // retrieve the exact expiration from cache so to not use a possibly expired
  // URL for 1h we use it for 5min and check again.
  return CreateAndCacheClient(*base_url, key, kLookupApiShortExpiryTime);
}

void ApiLookupClientImpl::PutToDiskCache(const ApisResult& available_services) {
  repository::ApiCacheRepository cache_repository_(catalog_, settings_.cache);
  for (const auto& service_api : available_services.first) {
    cache_repository_.Put(service_api.GetApi(), service_api.GetVersion(),
                          service_api.GetBaseUrl(), available_services.second);
  }
}

}  // namespace client
}  // namespace olp
