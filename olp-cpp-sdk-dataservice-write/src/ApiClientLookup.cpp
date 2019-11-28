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

#include "ApiClientLookup.h"

#include <olp/core/client/ApiError.h>
#include <olp/core/client/Condition.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/cache/KeyValueCache.h>
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
      {"here-dev", "data.api.platform.in.here.com"},
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
                      service_version.c_str(), hrn.partition.c_str());

  // compare the hrn
  auto base_url = GetDatastoreServerUrl(hrn.partition);
  if (base_url.empty()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s Lookup URL not found",
                       service.c_str(), service_version.c_str(),
                       hrn.partition.c_str());
    callback(
        client::ApiError(client::ErrorCode::NotFound, "Invalid or broken HRN"));
    return client::CancellationToken();
  }

  client->SetBaseUrl(base_url);

  if (service == "config") {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - config service",
                       service.c_str(), service_version.c_str(),
                       hrn.partition.c_str());

    // scan apis at platform apis
    return PlatformApi::GetApis(
        client, service, service_version,
        [callback](PlatformApi::ApisResponse response) { callback(response); });
  }

  OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - resource service",
                     service.c_str(), service_version.c_str(),
                     hrn.partition.c_str());

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
                      service_version.c_str(), hrn.partition.c_str());

  return ApiClientLookup::LookupApi(
      client, service, service_version, hrn, [=](ApisResponse response) {
        if (!response.IsSuccessful()) {
          OLP_SDK_LOG_INFO_F(
              kLogTag, "LookupApiClient(%s/%s): %s - unsuccessful: %s",
              service.c_str(), service_version.c_str(), hrn.partition.c_str(),
              response.GetError().GetMessage().c_str());

          callback(response.GetError());
        } else if (response.GetResult().size() < 1) {
          OLP_SDK_LOG_INFO_F(
              kLogTag, "LookupApiClient(%s/%s): %s - service not available",
              service.c_str(), service_version.c_str(), hrn.partition.c_str());

          // TODO use defined ErrorCode
          callback(
              client::ApiError(client::ErrorCode::ServiceUnavailable,
                               "Service/Version not available for given HRN"));
        } else {
          const auto& base_url = response.GetResult().at(0).GetBaseUrl();
          OLP_SDK_LOG_INFO_F(kLogTag,
                             "LookupApiClient(%s/%s): %s - OK, base_url=%s",
                             service.c_str(), service_version.c_str(),
                             hrn.partition.c_str(), base_url.c_str());

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
      client::OlpClient client;
      client.SetSettings(settings);
      client.SetBaseUrl(base_url);
      return ApiClientResponse{client};
    }
  }

  client::Condition condition;
  // when the network operation took too much time we cancel it and exit
  // execution, to make sure that network callback will not access dangling
  // references we protect them with atomic bool flag.
  auto flag = std::make_shared<std::atomic_bool>(true);

  ApiClientResponse api_response;
  auto api_callback = [&, flag](ApiClientResponse response) {
    if (flag->exchange(false)) {
      api_response = std::move(response);
      condition.Notify();
    }
  };

  cancellation_context.ExecuteOrCancelled(
      [&, flag]() {
        auto client_ptr = std::make_shared<olp::client::OlpClient>();
        client_ptr->SetSettings(settings);

        auto token = ApiClientLookup::LookupApiClient(
            client_ptr, service, service_version, catalog, api_callback);
        return client::CancellationToken([&, token, flag]() {
          if (flag->exchange(false)) {
            token.Cancel();
            condition.Notify();
          }
        });
      },
      [&]() {
        // if context was cancelled before the execution setup, unblock the
        // upcoming wait routine.
        condition.Notify();
      });
  if (!condition.Wait(std::chrono::seconds(settings.retry_settings.timeout))) {
    cancellation_context.CancelOperation();
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - timeout",
                       service.c_str(), service_version.c_str(),
                       catalog.partition.c_str());
    return client::ApiError(client::ErrorCode::RequestTimeout,
                            "Network request timed out.");
  }

  flag->store(false);

  if (cancellation_context.IsCancelled()) {
    // We can't use api response here because it could potentially be
    // uninitialized.
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - cancelled",
                       service.c_str(), service_version.c_str(),
                       catalog.partition.c_str());
    return client::ApiError(client::ErrorCode::Cancelled,
                            "Operation cancelled.");
  }

  if (!api_response.IsSuccessful()) {
    return api_response.GetError();
  }

  auto client = api_response.GetResult();
  if (client.GetBaseUrl().empty()) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "LookupApi(%s/%s): %s - empty base URL",
                          service.c_str(), service_version.c_str(),
                          catalog.partition.c_str());
  }

  if (cache) {
    const auto& base_url = client.GetBaseUrl();

    constexpr time_t kExpiryTimeInSecs = 3600;
    if (cache->Put(cache_key, base_url, [base_url]() { return base_url; },
                   kExpiryTimeInSecs)) {
      OLP_SDK_LOG_TRACE_F(kLogTag, "Put '%s' to cache", cache_key.c_str());
    } else {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Failed to put '%s' to cache",
                            cache_key.c_str());
    }
  }

  client.SetSettings(settings);
  return ApiClientResponse{std::move(client)};
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
