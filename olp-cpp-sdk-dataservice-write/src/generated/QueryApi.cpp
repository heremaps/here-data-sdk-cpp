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

#include "QueryApi.h"

#include <map>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>

// clang-format off
#include "generated/parser/PartitionsParser.h"
#include "JsonResultParser.h"
// clang-format on

namespace {
std::string concatStringArray(const std::vector<std::string>& strings,
                              const std::string& delimiter) {
  if (strings.size() == 1) {
    return strings.at(0);
  }

  std::stringstream buffer;
  std::copy(strings.begin(), strings.end() - 1,
            std::ostream_iterator<std::string>(buffer, delimiter.c_str()));
  buffer << *(strings.end() - 1);

  return buffer.str();
}

}  // namespace

namespace olp {
namespace dataservice {
namespace write {
namespace client = olp::client;

client::CancellationToken QueryApi::GetPartitionsById(
    const client::OlpClient& client, const std::string& layerId,
    const std::vector<std::string>& partitionIds,
    boost::optional<int64_t> version,
    boost::optional<std::vector<std::string>> additionalFields,
    boost::optional<std::string> billingTag,
    const PartitionsCallback& partitionsCallback) {
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("Accept", "application/json"));

  std::multimap<std::string, std::string> queryParams;
  queryParams.insert(std::make_pair(
      "partition", concatStringArray(partitionIds, "&partition=")));
  if (additionalFields) {
    queryParams.insert(std::make_pair(
        "additionalFields", concatStringArray(*additionalFields, ",")));
  }
  if (billingTag) {
    queryParams.insert(std::make_pair("billingTag", *billingTag));
  }
  if (version) {
    queryParams.insert(std::make_pair("version", std::to_string(*version)));
  }

  std::multimap<std::string, std::string> formParams;

  std::string queryUri = "/layers/" + layerId + "/partitions";

  client::NetworkAsyncCallback callback =
      [partitionsCallback](client::HttpResponse response) {
        if (response.status != http::HttpStatusCode::OK) {
          partitionsCallback(
              client::ApiError(response.status, response.response.str()));
        } else {
          partitionsCallback(
              parser::parse_result<PartitionsResponse>(response.response));
        }
      };

  return client.CallApi(queryUri, "GET", queryParams, headerParams, formParams,
                        nullptr, "", callback);
}

QueryApi::PartitionsResponse QueryApi::GetPartitionsById(
    const client::OlpClient& client, const std::string& layer_id,
    const std::vector<std::string>& partition_ids,
    boost::optional<int64_t> version,
    boost::optional<std::vector<std::string>> additional_fields,
    boost::optional<std::string> billing_tag,
    client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));

  std::multimap<std::string, std::string> query_params;
  query_params.insert(std::make_pair(
      "partition", concatStringArray(partition_ids, "&partition=")));
  if (additional_fields) {
    query_params.insert(std::make_pair(
        "additionalFields", concatStringArray(*additional_fields, ",")));
  }
  if (billing_tag) {
    query_params.insert(std::make_pair("billingTag", *billing_tag));
  }
  if (version) {
    query_params.insert(std::make_pair("version", std::to_string(*version)));
  }

  std::multimap<std::string, std::string> form_params;

  std::string query_uri = "/layers/" + layer_id + "/partitions";

  auto http_response =
      client.CallApi(query_uri, "GET", query_params, header_params, form_params,
                     nullptr, "", context);

  if (http_response.status != http::HttpStatusCode::OK) {
    return client::ApiError(http_response.status, http_response.response.str());
  }
  return parser::parse_result<PartitionsResponse>(http_response.response);
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
