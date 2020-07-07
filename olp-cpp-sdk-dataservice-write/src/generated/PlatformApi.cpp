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

#include "PlatformApi.h"

#include <memory>
#include <sstream>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
// clang-format off
#include "generated/parser/ApiParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace olp {
namespace dataservice {
namespace write {
client::CancellationToken PlatformApi::GetApis(
    std::shared_ptr<client::OlpClient> client, const std::string& service,
    const std::string& service_version, const ApisCallback& apisCallback) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  std::string platform_url =
      "/platform/apis/" + service + "/" + service_version;

  client::NetworkAsyncCallback callback = [apisCallback](
                                              client::HttpResponse response) {
    if (response.status != olp::http::HttpStatusCode::OK) {
      apisCallback(ApisResponse(
          client::ApiError(response.status, response.response.str())));
    } else {
      // parse the services
      // TODO catch any exception and return as Error
      apisCallback(ApisResponse(parser::parse<model::Apis>(response.response)));
    }
  };

  return client->CallApi(platform_url, "GET", query_params, header_params,
                         form_params, nullptr, "", callback);
}

PlatformApi::ApisResponse PlatformApi::GetApis(
    const client::OlpClient& client, const std::string& service,
    const std::string& service_version,
    client::CancellationContext cancel_context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  std::string platform_url =
      "/platform/apis/" + service + "/" + service_version;

  auto http_response =
      client.CallApi(std::move(platform_url), "GET", std::move(query_params),
                     std::move(header_params), std::move(form_params), nullptr,
                     "", cancel_context);
  if (http_response.status != olp::http::HttpStatusCode::OK) {
    return client::ApiError(http_response.status, http_response.response.str());
  }

  return ApisResponse(parser::parse<model::Apis>(http_response.response));
}

PlatformApi::ApisResponse PlatformApi::GetApis(
    const client::OlpClient& client, client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  std::string platform_url = "/platform/apis";

  auto http_response = client.CallApi(
      std::move(platform_url), "GET", std::move(query_params),
      std::move(header_params), std::move(form_params), nullptr, "", context);
  if (http_response.status != olp::http::HttpStatusCode::OK) {
    return {{http_response.status, http_response.response.str()}};
  }

  return parser::parse<model::Apis>(http_response.response);
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
