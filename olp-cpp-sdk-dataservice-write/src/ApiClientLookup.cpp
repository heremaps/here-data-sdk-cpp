/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/Condition.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/logging/Log.h>
#include "generated/PlatformApi.h"
#include "generated/ResourcesApi.h"

namespace olp {
namespace dataservice {
namespace write {

namespace {
constexpr auto kLogTag = "ApiClientLookupWrite";

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

std::string CreateKeyForCache(const std::string& hrn,
                              const std::string& service,
                              const std::string& service_version) {
  return hrn + "::" + service + "::" + service_version + "::api";
}

}  // namespace

client::CancellationToken ApiClientLookup::LookupApi(
    std::shared_ptr<client::OlpClient> client, const std::string& service,
    const std::string& service_version, const client::HRN& hrn,
    const ApisCallback& callback) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "LookupApi(%s/%s): %s", service.c_str(),
                      service_version.c_str(), hrn.GetPartition().c_str());

  // compare the hrn
  auto base_url = GetDatastoreServerUrl(hrn.GetPartition());
  if (base_url.empty()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s Lookup URL not found",
                       service.c_str(), service_version.c_str(),
                       hrn.GetPartition().c_str());
    callback(
        client::ApiError(client::ErrorCode::NotFound, "Invalid or broken HRN"));
    return client::CancellationToken();
  }

  client->SetBaseUrl(base_url);

  if (service == "config") {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - config service",
                       service.c_str(), service_version.c_str(),
                       hrn.GetPartition().c_str());

    // scan apis at platform apis
    return PlatformApi::GetApis(
        client, service, service_version,
        [callback](PlatformApi::ApisResponse response) { callback(response); });
  }

  OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - resource service",
                     service.c_str(), service_version.c_str(),
                     hrn.GetPartition().c_str());

  // scan apis at resource endpoint
  return ResourcesApi::GetApis(
      client, hrn.ToCatalogHRNString(), service, service_version,
      [callback](PlatformApi::ApisResponse response) { callback(response); });
}

client::CancellationToken ApiClientLookup::LookupApiClient(
    std::shared_ptr<client::OlpClient> client, const std::string& service,
    const std::string& service_version, const client::HRN& hrn,
    const ApiClientCallback& callback) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "LookupApiClient(%s/%s): %s", service.c_str(),
                      service_version.c_str(), hrn.GetPartition().c_str());

  return ApiClientLookup::LookupApi(
      client, service, service_version, hrn, [=](ApisResponse response) {
        if (!response.IsSuccessful()) {
          OLP_SDK_LOG_INFO_F(kLogTag,
                             "LookupApiClient(%s/%s): %s - unsuccessful: %s",
                             service.c_str(), service_version.c_str(),
                             hrn.GetPartition().c_str(),
                             response.GetError().GetMessage().c_str());

          callback(response.GetError());
        } else if (response.GetResult().size() < 1) {
          OLP_SDK_LOG_INFO_F(
              kLogTag, "LookupApiClient(%s/%s): %s - service not available",
              service.c_str(), service_version.c_str(),
              hrn.GetPartition().c_str());

          // TODO use defined ErrorCode
          callback(
              client::ApiError(client::ErrorCode::ServiceUnavailable,
                               "Service/Version not available for given HRN"));
        } else {
          const auto& base_url = response.GetResult().at(0).GetBaseUrl();
          OLP_SDK_LOG_INFO_F(kLogTag,
                             "LookupApiClient(%s/%s): %s - OK, base_url=%s",
                             service.c_str(), service_version.c_str(),
                             hrn.GetPartition().c_str(), base_url.c_str());

          client->SetBaseUrl(base_url);
          callback(*client);
        }
      });
}

ApiClientLookup::ApiClientResponse ApiClientLookup::LookupApiClient(
    const client::HRN& catalog,
    client::CancellationContext cancellation_context, std::string service,
    std::string service_version, client::OlpClientSettings settings) {
  auto cache_key =
      CreateKeyForCache(catalog.ToCatalogHRNString(), service, service_version);
  // first, try to find the corresponding Base URL from cache
  auto cache = settings.cache;
  if (cache) {
    auto url =
        cache->Get(cache_key, [](const std::string& value) { return value; });
    if (!url.empty()) {
      auto base_url = boost::any_cast<std::string>(url);
      OLP_SDK_LOG_INFO_F(kLogTag, "LookupApiClient(%s, %s) -> from cache",
                         service.c_str(), service_version.c_str());
      client::OlpClient client(settings, base_url);
      return ApiClientResponse{client};
    }
  }

  OLP_SDK_LOG_TRACE_F(kLogTag, "LookupApiClient(%s/%s): %s", service.c_str(),
                      service_version.c_str(), catalog.GetPartition().c_str());

  // compare the hrn
  const auto base_url = GetDatastoreServerUrl(catalog.GetPartition());
  if (base_url.empty()) {
    OLP_SDK_LOG_INFO_F(kLogTag,
                       "LookupApiClient(%s/%s): %s Lookup URL not found",
                       service.c_str(), service_version.c_str(),
                       catalog.GetPartition().c_str());
    return client::ApiError(client::ErrorCode::NotFound,
                            "Invalid or broken HRN");
  }

  ApisResponse api_response;
  client::OlpClient input_client(settings, base_url);

  if (service == "config") {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApiClient(%s/%s): %s - config service",
                       service.c_str(), service_version.c_str(),
                       catalog.GetPartition().c_str());

    // scan apis at platform apis
    api_response = PlatformApi::GetApis(input_client, service, service_version,
                                        cancellation_context);
  } else {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApiClient(%s/%s): %s - resource service",
                       service.c_str(), service_version.c_str(),
                       catalog.GetPartition().c_str());

    // scan apis at resource endpoint
    api_response =
        ResourcesApi::GetApis(input_client, catalog.ToCatalogHRNString(),
                              service, service_version, cancellation_context);
  }

  if (!api_response.IsSuccessful()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApiClient(%s/%s): %s - unsuccessful: %s",
                       service.c_str(), service_version.c_str(),
                       catalog.GetPartition().c_str(),
                       api_response.GetError().GetMessage().c_str());
    return api_response.GetError();
  } else if (api_response.GetResult().empty()) {
    OLP_SDK_LOG_INFO_F(kLogTag,
                       "LookupApiClient(%s/%s): %s - service not available",
                       service.c_str(), service_version.c_str(),
                       catalog.GetPartition().c_str());
    return client::ApiError(client::ErrorCode::ServiceUnavailable,
                            "Service/Version not available for given HRN");
  }

  const auto output_base_url = api_response.GetResult().front().GetBaseUrl();
  if (output_base_url.empty()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag, "LookupApiClient(%s/%s): %s - empty base URL", service.c_str(),
        service_version.c_str(), catalog.GetPartition().c_str());
  }

  if (cache) {
    constexpr time_t kExpiryTimeInSecs = 3600;
    if (cache->Put(cache_key, output_base_url,
                   [output_base_url]() { return output_base_url; },
                   kExpiryTimeInSecs)) {
      OLP_SDK_LOG_TRACE_F(kLogTag, "Put '%s' to cache", cache_key.c_str());
    } else {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Failed to put '%s' to cache",
                            cache_key.c_str());
    }
  }

  client::OlpClient output_client(settings, output_base_url);
  return ApiClientResponse{std::move(output_client)};
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
