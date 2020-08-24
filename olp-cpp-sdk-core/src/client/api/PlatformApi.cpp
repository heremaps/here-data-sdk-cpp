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

#include "PlatformApi.h"

#include <map>
#include <memory>
#include <sstream>
#include <utility>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/http/NetworkUtils.h>
// clang-format off
#include "client/parser/ApiParser.h"
#include "JsonResultParser.h"
// clang-format on

namespace {
using NetworkUtils = olp::http::NetworkUtils;
const std::string kExpiryHeader = "cache-control";

boost::optional<time_t> GetExpiry(const olp::http::Headers& headers) {
  auto it = std::find_if(headers.begin(), headers.end(),
                         [](const olp::http::Header& header) {
                           return NetworkUtils::CaseInsensitiveCompare(
                               header.first, kExpiryHeader);
                         });
  if (it == headers.end()) {
    return boost::none;
  }

  const auto& expiry_str = it->second;
  std::size_t index = NetworkUtils::CaseInsensitiveFind(expiry_str, "max-age=");
  if (index == std::string::npos) {
    return boost::none;
  }

  auto expiry = std::stoi(expiry_str.substr(index + 8));
  return expiry;
}

}  // namespace

namespace olp {
namespace client {

PlatformApi::ApisResponse PlatformApi::GetApis(
    const OlpClient& client, const CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));

  const auto platform_url = "/platform/apis";

  auto response = client.CallApi(platform_url, "GET", {}, header_params, {},
                                 nullptr, "", context);
  if (response.status != http::HttpStatusCode::OK) {
    return {{response.status, response.response.str()}};
  }

  return parse_result<ApisResponse, Apis>(response.response,
                                          GetExpiry(response.headers));
}

CancellationToken PlatformApi::GetApis(const OlpClient& client,
                                       const ApisCallback& callback) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));

  const auto platform_url = "/platform/apis";

  auto network_callback = [callback](client::HttpResponse response) {
    if (response.status != olp::http::HttpStatusCode::OK) {
      callback({{response.status, response.response.str()}});
    } else {
      callback(parse_result<ApisResponse, Apis>(response.response,
                                                GetExpiry(response.headers)));
    }
  };

  return client.CallApi(platform_url, "GET", {}, header_params, {}, nullptr, "",
                        network_callback);
}

}  // namespace client
}  // namespace olp
