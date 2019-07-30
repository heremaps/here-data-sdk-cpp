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
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/logging/Log.h>

#include "generated/api/PlatformApi.h"
#include "generated/api/ResourcesApi.h"

namespace olp {
namespace dataservice {
namespace read {
using namespace olp::client;

namespace {
constexpr auto kLogTag = "ApiClientLookup";

const std::map<std::string, std::string>& getDatastoreServerUrl() {
  static std::map<std::string, std::string> sDatastoreServerUrl = {
      {"here-dev", "data.api.platform.in.here.com"},
      {"here", "data.api.platform.here.com"}};

  return sDatastoreServerUrl;
}
}  // namespace

CancellationToken ApiClientLookup::LookupApi(std::shared_ptr<OlpClient> client,
                                             const std::string& service,
                                             const std::string& service_version,
                                             const HRN& hrn,
                                             const ApisCallback& callback) {
  EDGE_SDK_LOG_TRACE_F(kLogTag, "LookupApi(%s/%s): %s", service.c_str(),
                       service_version.c_str(), hrn.partition.c_str());
  // compare the hrn
  const auto& datastoreServerUrl = getDatastoreServerUrl();
  auto findResult = datastoreServerUrl.find(hrn.partition);

  if (findResult == datastoreServerUrl.cend()) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s could not find HRN",
                        service.c_str(), service_version.c_str(),
                        hrn.partition.c_str());
    callback(ApiError(ErrorCode::NotFound,
                      std::string()));  // TODO better error description?
    return CancellationToken();
  } else {
    client->SetBaseUrl("https://api-lookup." + findResult->second +
                       "/lookup/v1");

    if (service == "config") {
      // scan apis at platform apis
      EDGE_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - config service",
                          service.c_str(), service_version.c_str(),
                          hrn.partition.c_str());
      return PlatformApi::GetApis(
          client, service, service_version,
          [callback](PlatformApi::ApisResponse response) {
            callback(response);
          });
    } else {
      // scan apis at resource endpoint
      EDGE_SDK_LOG_INFO_F(kLogTag, "LookupApi(%s/%s): %s - resource service",
                          service.c_str(), service_version.c_str(),
                          hrn.partition.c_str());
      return ResourcesApi::GetApis(
          client, hrn.ToCatalogHRNString(), service, service_version,
          [callback](PlatformApi::ApisResponse response) {
            callback(response);
          });
    }
  }
}

CancellationToken ApiClientLookup::LookupApiClient(
    std::shared_ptr<OlpClient> client, const std::string& service,
    const std::string& service_version, const HRN& hrn,
    const ApiClientCallback& callback) {
  EDGE_SDK_LOG_TRACE_F(kLogTag, "LookupApiClient(%s/%s): %s", service.c_str(),
                       service_version.c_str(), hrn.partition.c_str());
  return ApiClientLookup::LookupApi(
      client, service, service_version, hrn,

      [client, callback, service, service_version, hrn](ApisResponse response) {
        if (!response.IsSuccessful()) {
          EDGE_SDK_LOG_INFO_F(
              kLogTag, "LookupApiClient(%s/%s): %s - unsuccessful: %s",
              service.c_str(), service_version.c_str(), hrn.partition.c_str(),
              response.GetError().GetMessage().c_str());
          callback(response.GetError());
        } else if (response.GetResult().size() < 1) {
          EDGE_SDK_LOG_INFO_F(
              kLogTag, "LookupApiClient(%s/%s): %s - service not available",
              service.c_str(), service_version.c_str(), hrn.partition.c_str());
          // TODO use defined ErrorCode
          callback(ApiError(ErrorCode::ServiceUnavailable,
                            "Service/Version not available for given HRN"));
        } else {
          EDGE_SDK_LOG_INFO_F(kLogTag, "LookupApiClient(%s/%s): %s - OK",
                              service.c_str(), service_version.c_str(),
                              hrn.partition.c_str());
          client->SetBaseUrl(response.GetResult().at(0).GetBaseUrl());
          callback(*(client.get()));
        }
      });
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
