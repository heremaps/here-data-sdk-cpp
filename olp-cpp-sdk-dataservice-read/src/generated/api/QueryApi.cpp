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

#include <algorithm>
#include <map>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/logging/Log.h>
// clang-format off
#include "generated/parser/IndexParser.h"
#include "generated/parser/PartitionsParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace {

constexpr auto kLogTag = "read::QueryApi";

std::string ConcatStringArray(const std::vector<std::string>& strings,
                              const std::string& delimiter) {
  if (strings.empty()) {
    return {};
  }
  if (strings.size() == 1) {
    return strings.front();
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
namespace read {

QueryApi::PartitionsResponse QueryApi::GetPartitionsbyId(
    const client::OlpClient& client, const std::string& layer_id,
    const std::vector<std::string>& partitions,
    boost::optional<int64_t> version,
    const std::vector<std::string>& additional_fields,
    boost::optional<std::string> billing_tag,
    client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));

  std::multimap<std::string, std::string> query_params;
  for (const auto& partition : partitions) {
    query_params.emplace("partition", partition);
  }
  if (!additional_fields.empty()) {
    query_params.insert(std::make_pair(
        "additionalFields", ConcatStringArray(additional_fields, ",")));
  }
  if (billing_tag) {
    query_params.insert(std::make_pair("billingTag", *billing_tag));
  }
  if (version) {
    query_params.insert(std::make_pair("version", std::to_string(*version)));
  }

  std::string metadata_uri = "/layers/" + layer_id + "/partitions";

  client::HttpResponse response = client.CallApi(
      metadata_uri, "GET", std::move(query_params), std::move(header_params),
      {}, nullptr, std::string{}, std::move(context));
  if (response.status != olp::http::HttpStatusCode::OK) {
    return client::ApiError(response.status, response.response.str());
  }

  OLP_SDK_LOG_TRACE_F(kLogTag, "GetPartitionsbyId, uri=%s, status=%d",
                      metadata_uri.c_str(), response.status);

  return olp::parser::parse_result<PartitionsResponse, model::Partitions>(
      response.response,
      client::ApiError(client::ErrorCode::Unknown, "Fail parsing responce."));
}

olp::client::HttpResponse QueryApi::QuadTreeIndex(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& quad_key, boost::optional<int64_t> version,
    int32_t depth, boost::optional<std::vector<std::string>> additional_fields,
    boost::optional<std::string> billing_tag,
    client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  std::multimap<std::string, std::string> query_params;
  if (additional_fields) {
    query_params.emplace("additionalFields",
                         ConcatStringArray(*additional_fields, ","));
  }
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }

  std::string metadata_uri =
      "/layers/" + layer_id +
      (version ? "/versions/" + std::to_string(version.get()) : "") +
      "/quadkeys/" + quad_key + "/depths/" + std::to_string(depth);

  return client.CallApi(metadata_uri, "GET", std::move(query_params),
                        std::move(header_params), {}, nullptr, std::string{},
                        std::move(context));
}

QueryApi::QuadTreeIndexResponse QueryApi::QuadTreeIndexVolatile(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& quad_key, int32_t depth,
    boost::optional<std::vector<std::string>> additional_fields,
    boost::optional<std::string> billing_tag,
    client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));

  std::multimap<std::string, std::string> query_params;
  if (additional_fields) {
    query_params.insert(std::make_pair(
        "additionalFields", ConcatStringArray(*additional_fields, ",")));
  }
  if (billing_tag) {
    query_params.insert(std::make_pair("billingTag", *billing_tag));
  }

  std::string metadata_uri = "/layers/" + layer_id + "/quadkeys/" + quad_key +
                             "/depths/" + std::to_string(depth);

  client::HttpResponse response = client.CallApi(
      metadata_uri, "GET", std::move(query_params), std::move(header_params),
      {}, nullptr, std::string{}, std::move(context));

  OLP_SDK_LOG_DEBUG_F(kLogTag, "QuadTreeIndex, uri=%s, status=%d",
                      metadata_uri.c_str(), response.status);
  if (response.status != olp::http::HttpStatusCode::OK) {
    return client::ApiError(response.status, response.response.str());
  }

  return olp::parser::parse_result<QuadTreeIndexResponse, model::Index>(
      response.response,
      client::ApiError(client::ErrorCode::Unknown, "Fail parsing responce."));
}

}  // namespace read

}  // namespace dataservice

}  // namespace olp
