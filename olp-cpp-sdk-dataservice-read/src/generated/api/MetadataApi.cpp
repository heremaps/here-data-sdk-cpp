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

#include "MetadataApi.h"

#include <algorithm>
#include <map>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>

// clang-format off
#include "generated/parser/VersionsResponseParser.h"
#include "generated/parser/LayerVersionsParser.h"
#include "generated/parser/PartitionsParser.h"
#include "generated/parser/VersionResponseParser.h"
#include "generated/parser/VersionInfosParser.h"
#include "JsonResultParser.h"
#include "generated/serializer/CatalogVersionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
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
namespace read {

MetadataApi::LayerVersionsResponse MetadataApi::GetLayerVersions(
    const client::OlpClient& client, std::int64_t version,
    porting::optional<std::string> billing_tag,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  std::multimap<std::string, std::string> query_params;
  query_params.emplace("version", std::to_string(version));
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }

  std::string metadataUri = "/layerVersions";

  auto api_response =
      client.CallApi(std::move(metadataUri), "GET", query_params, header_params,
                     {}, nullptr, "", context);

  if (api_response.GetStatus() != http::HttpStatusCode::OK) {
    return client::ApiError(api_response.GetStatus(),
                            api_response.GetResponseAsString());
  }

  return parser::parse_result<LayerVersionsResponse>(
      api_response.GetRawResponse());
}

MetadataApi::PartitionsExtendedResponse MetadataApi::GetPartitions(
    const client::OlpClient& client, const std::string& layer_id,
    porting::optional<std::int64_t> version,
    const std::vector<std::string>& additional_fields,
    porting::optional<std::string> range,
    porting::optional<std::string> billing_tag,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");
  if (range) {
    header_params.emplace("Range", *range);
  }

  std::multimap<std::string, std::string> query_params;
  if (!additional_fields.empty()) {
    query_params.emplace("additionalFields",
                         concatStringArray(additional_fields, ","));
  }
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }
  if (version) {
    query_params.emplace("version", std::to_string(*version));
  }

  std::string metadataUri = "/layers/" + layer_id + "/partitions";

  auto http_response =
      client.CallApi(std::move(metadataUri), "GET", query_params, header_params,
                     {}, nullptr, "", context);

  if (http_response.GetStatus() != olp::http::HttpStatusCode::OK) {
    return PartitionsExtendedResponse(
        client::ApiError(http_response.GetStatus(),
                         http_response.GetResponseAsString()),
        http_response.GetNetworkStatistics());
  }

  using PartitionsResponse =
      client::ApiResponse<model::Partitions, client::ApiError>;

  auto partitions_response =
      parser::parse_result<PartitionsResponse>(http_response.GetRawResponse());

  if (!partitions_response.IsSuccessful()) {
    return PartitionsExtendedResponse(partitions_response.GetError(),
                                      http_response.GetNetworkStatistics());
  }

  return PartitionsExtendedResponse(partitions_response.MoveResult(),
                                    http_response.GetNetworkStatistics());
}

client::HttpResponse MetadataApi::GetPartitionsStream(
    const client::OlpClient& client, const std::string& layer_id,
    porting::optional<int64_t> version,
    const std::vector<std::string>& additional_fields,
    porting::optional<std::string> range,
    porting::optional<std::string> billing_tag,
    http::Network::DataCallback data_callback,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");
  if (range) {
    header_params.emplace("Range", *range);
  }

  std::multimap<std::string, std::string> query_params;
  if (!additional_fields.empty()) {
    query_params.emplace("additionalFields",
                         concatStringArray(additional_fields, ","));
  }
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }
  if (version) {
    query_params.emplace("version", std::to_string(*version));
  }

  std::string metadataUri = "/layers/" + layer_id + "/partitions";

  return client.CallApiStream(std::move(metadataUri), "GET", query_params,
                              header_params, std::move(data_callback), nullptr,
                              "", context);
}

MetadataApi::CatalogVersionResponse MetadataApi::GetLatestCatalogVersion(
    const client::OlpClient& client, std::int64_t startVersion,
    porting::optional<std::string> billing_tag,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  std::multimap<std::string, std::string> query_params;
  query_params.emplace("startVersion", std::to_string(startVersion));
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }

  std::string metadata_uri = "/versions/latest";

  auto api_response =
      client.CallApi(std::move(metadata_uri), "GET", query_params,
                     header_params, {}, nullptr, "", context);

  if (api_response.GetStatus() != http::HttpStatusCode::OK) {
    return {{api_response.GetStatus(), api_response.GetResponseAsString()}};
  }

  return parser::parse_result<CatalogVersionResponse>(
      api_response.GetRawResponse());
}

MetadataApi::VersionsResponse MetadataApi::ListVersions(
    const client::OlpClient& client, std::int64_t start_version,
    std::int64_t end_version, porting::optional<std::string> billing_tag,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  std::multimap<std::string, std::string> query_params;
  query_params.emplace("startVersion", std::to_string(start_version));
  query_params.emplace("endVersion", std::to_string(end_version));
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }

  std::string metadata_uri = "/versions";

  auto api_response = client.CallApi(metadata_uri, "GET", query_params,
                                     header_params, {}, nullptr, "", context);

  if (api_response.GetStatus() != http::HttpStatusCode::OK) {
    return {{api_response.GetStatus(), api_response.GetResponseAsString()}};
  }
  return parser::parse_result<VersionsResponse>(api_response.GetRawResponse());
}

MetadataApi::CompatibleVersionsResponse MetadataApi::GetCompatibleVersions(
    const client::OlpClient& client, const CatalogVersions& dependencies,
    int32_t limit, const client::CancellationContext& context) {
  std::string metadata_uri = "/versions/compatibles";

  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  std::multimap<std::string, std::string> query_params;
  query_params.emplace("limit", std::to_string(limit));

  rapidjson::Value value;

  const auto serialized_dependencies = serializer::serialize(dependencies);

  const auto data = std::make_shared<std::vector<unsigned char>>(
      serialized_dependencies.begin(), serialized_dependencies.end());

  auto api_response =
      client.CallApi(metadata_uri, "POST", query_params, header_params, {},
                     data, "application/json", context);

  if (api_response.GetStatus() != http::HttpStatusCode::OK) {
    return {{api_response.GetStatus(), api_response.GetResponseAsString()}};
  }

  return parser::parse_result<CompatibleVersionsResponse>(
      api_response.GetRawResponse());
}

}  // namespace read

}  // namespace dataservice

}  // namespace olp
