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

#include <olp/core/logging/Log.h>

#include <olp/core/client/OlpClient.h>
#include <olp/core/network/HttpResponse.h>
// clang-format off
#include "generated/parser/IndexParser.h"
#include "generated/parser/PartitionsParser.h"
#include <olp/core/generated/parser/JsonParser.h>
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

#define LOGTAG "QUERY_API"

using namespace olp::client;

CancellationToken QueryApi::GetPartitionsbyId(
    const OlpClient& client, const std::string& layerId,
    const std::vector<std::string>& partitions,
    boost::optional<int64_t> version,
    boost::optional<std::vector<std::string>> additionalFields,
    boost::optional<std::string> billingTag,
    const PartitionsCallback& partitionsCallback) {
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("Accept", "application/json"));

  std::multimap<std::string, std::string> queryParams;
  for (const auto& partition : partitions) {
    queryParams.emplace("partition", partition);
  }
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

  std::string metadataUri = "/layers/" + layerId + "/partitions";

  NetworkAsyncCallback callback =
      [partitionsCallback](network::HttpResponse response) {
        if (response.status != 200) {
          partitionsCallback(ApiError(response.status, response.response));
        } else {
          partitionsCallback(
              olp::parser::parse<model::Partitions>(response.response));
        }
      };

  return client.CallApi(metadataUri, "GET", queryParams, headerParams,
                        formParams, nullptr, "", callback);
}

client::CancellationToken QueryApi::QuadTreeIndex(
    const client::OlpClient& client, const std::string& layerId,
    int64_t version, const std::string& quadKey, int32_t depth,
    boost::optional<std::vector<std::string>> additionalFields,
    boost::optional<std::string> billingTag,
    const QuadTreeIndexCallback& indexCallback) {
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("Accept", "application/json"));

  std::multimap<std::string, std::string> queryParams;
  if (additionalFields) {
    queryParams.insert(std::make_pair(
        "additionalFields", concatStringArray(*additionalFields, ",")));
  }
  if (billingTag) {
    queryParams.insert(std::make_pair("billingTag", *billingTag));
  }

  std::multimap<std::string, std::string> formParams;

  std::string metadataUri = "/layers/" + layerId + "/versions/" +
                            std::to_string(version) + "/quadkeys/" + quadKey +
                            "/depths/" + std::to_string(depth);

  NetworkAsyncCallback callback =
      [indexCallback](network::HttpResponse response) {
        EDGE_SDK_LOG_TRACE_F(LOGTAG, "QuadTreeIndex callback, status=%d",
                             response.status);
        if (response.status != 200) {
          indexCallback(ApiError(response.status, response.response));
        } else {
          indexCallback(olp::parser::parse<model::Index>(response.response));
        }
      };

  EDGE_SDK_LOG_TRACE(LOGTAG, "QuadTreeIndex");

  return client.CallApi(metadataUri, "GET", queryParams, headerParams,
                        formParams, nullptr, "", callback);
}

}  // namespace read

}  // namespace dataservice

}  // namespace olp
