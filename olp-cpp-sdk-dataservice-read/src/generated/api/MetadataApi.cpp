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

#include "MetadataApi.h"

#include <algorithm>
#include <map>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>

// clang-format off
#include "generated/parser/LayerVersionsParser.h"
#include "generated/parser/PartitionsParser.h"
#include "generated/parser/VersionResponseParser.h"
#include "generated/parser/VersionInfosParser.h"
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
namespace read {

MetadataApi::LayerVersionsResponse MetadataApi::GetLayerVersions(
    const client::OlpClient& client, std::int64_t version,
    boost::optional<std::string> billing_tag,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  std::multimap<std::string, std::string> query_params;
  query_params.emplace("version", std::to_string(version));
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }

  std::string metadataUri = "/layerVersions";

  auto api_response = client.CallApi(metadataUri, "GET", query_params,
                                     header_params, {}, nullptr, "", context);

  if (api_response.status != http::HttpStatusCode::OK) {
    return client::ApiError(api_response.status, api_response.response.str());
  }

  return parse_result<LayerVersionsResponse, model::LayerVersions>(
      api_response.response);
}

MetadataApi::PartitionsResponse MetadataApi::GetPartitions(
    const client::OlpClient& client, const std::string& layer_id,
    boost::optional<std::int64_t> version,
    const std::vector<std::string>& additional_fields,
    boost::optional<std::string> range,
    boost::optional<std::string> billing_tag,
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

  auto api_response = client.CallApi(metadataUri, "GET", query_params,
                                     header_params, {}, nullptr, "", context);

  if (api_response.status != http::HttpStatusCode::OK) {
    return {{api_response.status, api_response.response.str()}};
  }

  return parse_result<PartitionsResponse, model::Partitions>(
      api_response.response);
}

MetadataApi::CatalogVersionResponse MetadataApi::GetLatestCatalogVersion(
    const client::OlpClient& client, std::int64_t startVersion,
    boost::optional<std::string> billing_tag,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  std::multimap<std::string, std::string> query_params;
  query_params.emplace("startVersion", std::to_string(startVersion));
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }

  std::string metadata_uri = "/versions/latest";

  auto api_response = client.CallApi(metadata_uri, "GET", query_params,
                                     header_params, {}, nullptr, "", context);

  if (api_response.status != http::HttpStatusCode::OK) {
    return {{api_response.status, api_response.response.str()}};
  }

  return parse_result<CatalogVersionResponse, model::VersionResponse>(
      api_response.response);
}

MetadataApi::VersionsResponse MetadataApi::ListVersions(
    const client::OlpClient& client, std::int64_t start_version,
    std::int64_t end_version, boost::optional<std::string> billing_tag,
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

  if (api_response.status != http::HttpStatusCode::OK) {
    return {{api_response.status, api_response.response.str()}};
  }
  return parse_result<VersionsResponse, model::VersionInfos>(
      api_response.response);
}

}  // namespace read

}  // namespace dataservice

}  // namespace olp
