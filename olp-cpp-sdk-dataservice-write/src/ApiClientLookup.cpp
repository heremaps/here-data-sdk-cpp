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
#include "generated/PlatformApi.h"
#include "generated/ResourcesApi.h"

namespace olp {
namespace dataservice {
namespace write {
using namespace olp::client;
const std::map<std::string, std::string>& getDatastoreServerUrl() {
  static std::map<std::string, std::string> sDatastoreServerUrl = {
      {"here-dev", "data.api.platform.in.here.com"},
      {"here", "data.api.platform.here.com"}};

  return sDatastoreServerUrl;
}

client::CancellationToken ApiClientLookup::LookupApi(
    std::shared_ptr<client::OlpClient> client, const std::string& service,
    const std::string& service_version, const client::HRN& hrn,
    const ApisCallback& callback) {
  // compare the hrn
  const auto& datastoreServerUrl = getDatastoreServerUrl();
  auto findResult = datastoreServerUrl.find(hrn.partition);
  if (findResult == datastoreServerUrl.cend()) {
    callback(ApiError(ErrorCode::NotFound, std::string()));
    return CancellationToken();
  } else {
    client->SetBaseUrl("https://api-lookup." + findResult->second +
                       "/lookup/v1");

    if (service == "config") {
      // scan apis at platform apis
      return PlatformApi::GetApis(
          client, service, service_version,
          [callback](PlatformApi::ApisResponse response) {
            callback(response);
          });
    } else {
      // scan apis at resource endpoint
      return ResourcesApi::GetApis(
          client, hrn.ToCatalogHRNString(), service, service_version,
          [callback](PlatformApi::ApisResponse response) {
            callback(response);
          });
    }
  }
}

client::CancellationToken ApiClientLookup::LookupApiClient(
    std::shared_ptr<client::OlpClient> client, const std::string& service,
    const std::string& service_version, const client::HRN& hrn,
    const ApiClientCallback& callback) {
  return ApiClientLookup::LookupApi(
      client, service, service_version, hrn,

      [client, callback](ApisResponse response) {
        if (!response.IsSuccessful()) {
          callback(response.GetError());
        } else if (response.GetResult().size() < 1) {
          callback(ApiError(ErrorCode::ServiceUnavailable,
                            "Service/Version not available for given HRN"));
        } else {
          client->SetBaseUrl(response.GetResult().at(0).GetBaseUrl());
          callback(*(client.get()));
        }
      });
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
