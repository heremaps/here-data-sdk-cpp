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
      {"here-dev", "data.api.platform.in.here.com"},
      {"here-cn", "data.api.platform.hereolp.cn"},
      {"here-cn-dev", "data.api.platform.in.hereolp.cn"}};

  const auto& server_url = kDatastoreServerUrl.find(partition);
  return (server_url != kDatastoreServerUrl.end())
             ? "https://api-lookup." + server_url->second + "/lookup/v1"
             : "";
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

ApiClientLookup::ApiClientResponse ApiClientLookup::LookupApi(
    const client::HRN& catalog,
    client::CancellationContext cancellation_context, std::string service,
    std::string service_version, FetchOptions options,
    client::OlpClientSettings settings) {
  repository::ApiCacheRepository repository(catalog, settings.cache);

  if (options != OnlineOnly) {
    auto url = repository.Get(service, service_version);
    if (url) {
      OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s, %s) -> from cache",
                         service.c_str(), service_version.c_str());
      client::OlpClient client;
      client.SetSettings(std::move(settings));
      client.SetBaseUrl(*url);
      return std::move(client);
    } else if (options == CacheOnly) {
      return client::ApiError(
          client::ErrorCode::NotFound,
          "Cache only resource not found in cache (loopup api).");
    }
  }

  client::Condition condition{};

  // when the network operation took too much time we cancel it and exit
  // execution, to make sure that network callback will not access dangling
  // references we protect them with atomic bool flag.
  auto flag = std::make_shared<std::atomic_bool>(true);

  PlatformApi::ApisResponse api_response;
  auto api_callback = [&, flag](PlatformApi::ApisResponse response) {
    if (flag->exchange(false)) {
      api_response = std::move(response);
      condition.Notify();
    }
  };

  cancellation_context.ExecuteOrCancelled(
      [&, flag]() {
        const auto& base_url = GetDatastoreServerUrl(catalog.partition);

        client::OlpClient client;
        client.SetBaseUrl(base_url);
        client.SetSettings(std::move(settings));

        client::CancellationToken token;
        if (service == "config") {
          token = PlatformApi::GetApis(client, service, service_version,
                                       api_callback);
        } else {
          token = ResourcesApi::GetApis(client, catalog.ToCatalogHRNString(),
                                        service, service_version, api_callback);
        }

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
    return client::ApiError(client::ErrorCode::RequestTimeout,
                            "Network request timed out.");
  }

  flag->store(false);

  if (cancellation_context.IsCancelled()) {
    // We can't use api response here because it could potentially be
    // uninitialized.
    return client::ApiError(client::ErrorCode::Cancelled,
                            "Operation cancelled.");
  }

  if (!api_response.IsSuccessful()) {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - unsuccessful: %s",
                       service.c_str(), service_version.c_str(),
                       catalog.partition.c_str(),
                       api_response.GetError().GetMessage().c_str());
    return api_response.GetError();
  }

  const auto& api_result = api_response.GetResult();

  if (api_result.size() < 1) {
    OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - service not available",
                       service.c_str(), service_version.c_str(),
                       catalog.partition.c_str());

    return client::ApiError(client::ErrorCode::ServiceUnavailable,
                            "Service/Version not available for given HRN");
  }

  const auto& base_url = api_result.at(0).GetBaseUrl();

  repository.Put(service, service_version, base_url);

  OLP_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - OK, base_url=%s",
                     service.c_str(), service_version.c_str(),
                     catalog.partition.c_str(), base_url.c_str());

  client::OlpClient client;
  client.SetBaseUrl(base_url);
  client.SetSettings(std::move(settings));

  return std::move(client);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
