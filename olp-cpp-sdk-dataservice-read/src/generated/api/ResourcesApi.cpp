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

#include "ResourcesApi.h"

#include <map>
#include <memory>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
// clang-format off
#include "generated/parser/ApiParser.h"
#include "JsonResultParser.h"
// clang-format on

namespace olp {
namespace dataservice {
namespace read {

ResourcesApi::ApisResponse ResourcesApi::GetApis(
    const client::OlpClient& client, const std::string& hrn,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;
  // scan apis at resource endpoint
  const std::string resource_url = "/resources/" + hrn + "/apis";
  auto response =
      client.CallApi(resource_url, "GET", query_params, header_params,
                     form_params, nullptr, "", context);

  if (response.GetStatus() != http::HttpStatusCode::OK) {
    return ApisResponse(
        client::ApiError(response.GetStatus(), response.GetResponseAsString()));
  }

  return parser::parse_result<ApisResponse>(response.GetRawResponse());
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
