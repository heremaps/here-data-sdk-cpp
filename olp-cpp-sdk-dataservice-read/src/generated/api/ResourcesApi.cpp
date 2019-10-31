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

#include "ResourcesApi.h"

#include <memory>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
// clang-format off
#include "generated/parser/ApiParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace olp {
namespace dataservice {
namespace read {

client::CancellationToken ResourcesApi::GetApis(
    std::shared_ptr<client::OlpClient> client, const std::string& hrn,
    const std::string& service, const std::string& service_version,
    const ApisCallback& apisCallback) {
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("Accept", "application/json"));
  std::multimap<std::string, std::string> queryParams;
  std::multimap<std::string, std::string> formParams;

  // scan apis at resource endpoint
  std::string resourceUrl =
      "/resources/" + hrn + "/apis/" + service + "/" + service_version;

  client::NetworkAsyncCallback callback =
      [apisCallback](client::HttpResponse response) {
        if (response.status != 200) {
          apisCallback(ApisResponse(
              client::ApiError(response.status, response.response.str())));
        } else {
          // parse the services
          // TODO catch any exception and return as Error
          apisCallback(
              ApisResponse(parser::parse<olp::dataservice::read::model::Apis>(
                  response.response)));
        }
      };
  return client->CallApi(resourceUrl, "GET", queryParams, headerParams,
                         formParams, nullptr, "", callback);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
