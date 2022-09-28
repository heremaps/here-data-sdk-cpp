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

#include "ApiClientLookup.h"

#include <map>
#include <utility>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/Condition.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/logging/Log.h>
#include "generated/api/PlatformApi.h"
#include "generated/api/ResourcesApi.h"
#include "repositories/ApiCacheRepository.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "ApiClientLookupRead";

std::string GetDatastoreServerUrl(const std::string& partition) {
  static const std::map<std::string, std::string> kDatastoreServerUrl = {
      {"here", "data.api.platform.here.com"},
      {"here-dev", "data.api.platform.sit.here.com"},
      {"here-cn", "data.api.platform.hereolp.cn"},
      {"here-cn-dev", "data.api.platform.in.hereolp.cn"}};

  const auto& server_url = kDatastoreServerUrl.find(partition);
  return (server_url != kDatastoreServerUrl.end())
             ? "https://api-lookup." + server_url->second + "/lookup/v1"
             : "";
}
}  // namespace

ApiClientLookup::ApiClientResponse ApiClientLookup::LookupApi(
    const client::HRN& catalog,
    client::CancellationContext cancellation_context, std::string service,
    std::string service_version, FetchOptions options,
    client::OlpClientSettings settings, repository::NamedMutexStorage storage) {
  // This mutex is required to avoid concurrent requests to online.
  repository::NamedMutex mutex(storage, catalog.ToString(),
                               cancellation_context);
  std::unique_lock<repository::NamedMutex> lock(mutex, std::defer_lock);

  // If we are not planning to go online or access the cache, do not lock.
  if (options != CacheOnly && options != OnlineOnly) {
    lock.lock();
  }

  repository::ApiCacheRepository repository(catalog, settings.cache);
  const auto hrn = catalog.ToCatalogHRNString();

  if (options != OnlineOnly && options != CacheWithUpdate) {
    auto url = repository.Get(service, service_version);
    if (url) {
      OLP_SDK_LOG_DEBUG_F(kLogTag, "LookupApi(%s/%s) found in cache, hrn='%s'",
                          service.c_str(), service_version.c_str(),
                          hrn.c_str());
      client::OlpClient client;
      client.SetSettings(std::move(settings));
      client.SetBaseUrl(*url);
      return client;
    } else if (options == CacheOnly) {
      return {{client::ErrorCode::NotFound,
               "CacheOnly: resource not found in cache"}};
    }
  }

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "LookupApi(%s/%s) cache miss, requesting, hrn='%s'",
                     service.c_str(), service_version.c_str(), hrn.c_str());

  const auto& base_url = GetDatastoreServerUrl(catalog.GetPartition());
  client::OlpClient client;
  client.SetBaseUrl(base_url);
  // Do not move settings, we still need them later on!
  client.SetSettings(settings);
  PlatformApi::ApisResponse api_response;

  if (service == "config") {
    api_response = PlatformApi::GetApis(client, cancellation_context);
  } else {
    api_response = ResourcesApi::GetApis(client, hrn, cancellation_context);
  }

  if (!api_response.IsSuccessful()) {
    OLP_SDK_LOG_INFO_F(kLogTag,
                       "LookupApi(%s/%s) unsuccessful, hrn='%s', error='%s'",
                       service.c_str(), service_version.c_str(), hrn.c_str(),
                       api_response.GetError().GetMessage().c_str());
    return api_response.GetError();
  }

  const auto& api_result = api_response.GetResult();
  if (options != OnlineOnly && options != CacheWithUpdate) {
    for (const auto& service_api : api_result) {
      repository.Put(service_api.GetApi(), service_api.GetVersion(),
                     service_api.GetBaseUrl());
    }
  }

  auto it =
      std::find_if(api_result.begin(), api_result.end(),
                   [&](const dataservice::read::model::Api& obj) -> bool {
                     return (obj.GetApi().compare(service) == 0 &&
                             obj.GetVersion().compare(service_version) == 0);
                   });
  if (it == api_result.end()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag, "LookupApi(%s/%s) service not found, hrn='%s'",
        service.c_str(), service_version.c_str(), hrn.c_str());

    return {{client::ErrorCode::ServiceUnavailable,
             "Service/Version not available for given HRN"}};
  }

  const auto& service_url = it->GetBaseUrl();

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "LookupApi(%s/%s) found, hrn='%s', service_url='%s'",
                      service.c_str(), service_version.c_str(), hrn.c_str(),
                      service_url.c_str());

  client::OlpClient service_client;
  service_client.SetBaseUrl(service_url);
  service_client.SetSettings(settings);
  return service_client;
}
}  // namespace read
}  // namespace dataservice
}  // namespace olp
