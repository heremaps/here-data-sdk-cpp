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

#include "StreamApi.h"

#include <algorithm>
#include <map>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/model/Data.h>
// clang-format off
#include "generated/parser/MessagesParser.h"
#include "generated/parser/StreamOffsetParser.h"
#include "generated/parser/SubscribeResponseParser.h"
#include <olp/core/generated/parser/JsonParser.h>
#include "generated/serializer/ConsumerPropertiesSerializer.h"
#include "generated/serializer/StreamOffsetsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on

namespace {
constexpr auto kLogTag = "read::StreamApi";

void HandleCorrelationId(const olp::http::Headers& headers,
                         std::string& x_correlation_id) {
  auto it =
      std::find_if(std::begin(headers), std::end(headers),
                   [&](const olp::http::Header& header) {
                     return (header.first.compare("X-Correlation-Id") == 0);
                   });
  if (it != headers.end()) {
    x_correlation_id = it->second;
  }
}

}  // namespace

namespace olp {
namespace dataservice {
namespace read {

StreamApi::SubscribeApiResponse StreamApi::Subscribe(
    const client::OlpClient& client, const std::string& layer_id,
    const boost::optional<std::string>& subscription_id,
    const boost::optional<std::string>& mode,
    const boost::optional<std::string>& consumer_id,
    const boost::optional<ConsumerProperties>& subscription_properties,
    const client::CancellationContext& context, std::string& x_correlation_id) {
  const std::string metadata_uri = "/layers/" + layer_id + "/subscribe";

  std::multimap<std::string, std::string> query_params;
  if (subscription_id) {
    query_params.emplace("subscriptionId", subscription_id.get());
  }

  if (mode) {
    query_params.emplace("mode", mode.get());
  }

  if (consumer_id) {
    query_params.emplace("consumerId", consumer_id.get());
  }

  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  model::Data data;
  if (subscription_properties) {
    const auto serialized_subscription_properties =
        serializer::serialize(subscription_properties.get());
    data = std::make_shared<std::vector<unsigned char>>(
        serialized_subscription_properties.begin(),
        serialized_subscription_properties.end());
  }

  auto http_response = client.CallApi(
      metadata_uri, "POST", std::move(query_params), std::move(header_params),
      {}, data, "application/json", std::move(context));
  if (http_response.status != http::HttpStatusCode::CREATED) {
    return client::ApiError(http_response.status, http_response.response.str());
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag, "subscribe, uri=%s, status=%d",
                      metadata_uri.c_str(), http_response.status);

  HandleCorrelationId(http_response.headers, x_correlation_id);
  return parser::parse<model::SubscribeResponse>(http_response.response);
}

StreamApi::ConsumeDataApiResponse StreamApi::ConsumeData(
    const client::OlpClient& client, const std::string& layer_id,
    const boost::optional<std::string>& subscription_id,
    const boost::optional<std::string>& mode,
    const client::CancellationContext& context,
    std::string& x_correlation_id) {
  const std::string metadata_uri = "/layers/" + layer_id + "/partitions";

  std::multimap<std::string, std::string> query_params;
  if (subscription_id) {
    query_params.emplace("subscriptionId", subscription_id.get());
  }

  if (mode) {
    query_params.emplace("mode", mode.get());
  }

  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");
  header_params.emplace("X-Correlation-Id", x_correlation_id);

  auto http_response = client.CallApi(
      metadata_uri, "GET", std::move(query_params), std::move(header_params),
      {}, nullptr, std::string{}, std::move(context));
  if (http_response.status != http::HttpStatusCode::OK) {
    return client::ApiError(http_response.status, http_response.response.str());
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag, "consumeData, uri=%s, status=%d",
                      metadata_uri.c_str(), http_response.status);

  HandleCorrelationId(http_response.headers, x_correlation_id);
  return parser::parse<model::Messages>(http_response.response);
}

StreamApi::CommitOffsetsApiResponse StreamApi::CommitOffsets(
    const client::OlpClient& client, const std::string& layer_id,
    const model::StreamOffsets& commit_offsets,
    const boost::optional<std::string>& subscription_id,
    const boost::optional<std::string>& mode,
    const client::CancellationContext& context, std::string& x_correlation_id) {
  return HandleOffsets(client, layer_id, commit_offsets, subscription_id, mode,
                       context, "offsets", x_correlation_id);
}

StreamApi::SeekToOffsetApiResponse StreamApi::SeekToOffset(
    const client::OlpClient& client, const std::string& layer_id,
    const model::StreamOffsets& commit_offsets,
    const boost::optional<std::string>& subscription_id,
    const boost::optional<std::string>& mode,
    const client::CancellationContext& context, std::string& x_correlation_id) {
  return HandleOffsets(client, layer_id, commit_offsets, subscription_id, mode,
                       context, "seek", x_correlation_id);
}

StreamApi::UnsubscribeApiResponse StreamApi::DeleteSubscription(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& subscription_id, const std::string& mode,
    const std::string& x_correlation_id,
    const client::CancellationContext& context) {
  const std::string metadata_uri = "/layers/" + layer_id + "/subscribe";

  std::multimap<std::string, std::string> query_params;
  query_params.emplace("subscriptionId", subscription_id);

  query_params.emplace("mode", mode);

  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");

  header_params.emplace("X-Correlation-Id", x_correlation_id);

  const auto http_response = client.CallApi(
      metadata_uri, "DELETE", std::move(query_params), std::move(header_params),
      {}, nullptr, std::string{}, std::move(context));
  if (http_response.status != http::HttpStatusCode::OK) {
    return client::ApiError(http_response.status, http_response.response.str());
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag, "deleteSubscription, uri=%s, status=%d",
                      metadata_uri.c_str(), http_response.status);

  return http_response.status;
}

Response<int> StreamApi::HandleOffsets(
    const client::OlpClient& client, const std::string& layer_id,
    const model::StreamOffsets& offsets,
    const boost::optional<std::string>& subscription_id,
    const boost::optional<std::string>& mode,
    const client::CancellationContext& context, const std::string& endpoint,
    std::string& x_correlation_id) {
  const std::string metadata_uri = "/layers/" + layer_id + "/" + endpoint;

  std::multimap<std::string, std::string> query_params;
  if (subscription_id) {
    query_params.emplace("subscriptionId", subscription_id.get());
  }

  if (mode) {
    query_params.emplace("mode", mode.get());
  }

  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");
  header_params.emplace("X-Correlation-Id", x_correlation_id);

  const auto serialized_offsets = serializer::serialize(offsets);
  const auto data = std::make_shared<std::vector<unsigned char>>(
      serialized_offsets.begin(), serialized_offsets.end());

  const auto http_response = client.CallApi(
      metadata_uri, "PUT", std::move(query_params), std::move(header_params),
      {}, data, "application/json", std::move(context));
  if (http_response.status != http::HttpStatusCode::OK) {
    return client::ApiError(http_response.status, http_response.response.str());
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag, "handleOffsets, uri=%s, status=%d",
                      metadata_uri.c_str(), http_response.status);

  HandleCorrelationId(http_response.headers, x_correlation_id);
  return http_response.status;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
