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

#include "IngestApi.h"

#include <map>
#include <memory>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/write/generated/model/ResponseOk.h>
#include <olp/dataservice/write/generated/model/ResponseOkSingle.h>
// clang-format off
// Ordering Required - Parser template specializations before JsonParser.h
#include "parser/ResponseOkParser.h"
#include "parser/ResponseOkSingleParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

using namespace olp::client;

namespace {
const std::string kHeaderParamChecksum = "X-HERE-Checksum";
const std::string kHeaderParamTraceId = "X-HERE-TraceId";
const std::string kQueryParamBillingTag = "billingTag";

constexpr auto kLogTag = "IngestApi";
}  // namespace

namespace olp {
namespace dataservice {
namespace write {

client::CancellationToken IngestApi::IngestData(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& content_type,
    const std::shared_ptr<std::vector<unsigned char>>& data,
    const boost::optional<std::string>& trace_id,
    const boost::optional<std::string>& billing_tag,
    const boost::optional<std::string>& checksum, IngestDataCallback callback) {
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;
  std::multimap<std::string, std::string> header_params;

  header_params.insert(std::make_pair("Accept", "application/json"));
  if (trace_id) {
    header_params.insert(std::make_pair(kHeaderParamTraceId, trace_id.get()));
  }
  if (checksum) {
    header_params.insert(std::make_pair(kHeaderParamChecksum, checksum.get()));
  }

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string ingest_uri = "/layers/" + layer_id;

  auto cancel_token = client.CallApi(
      ingest_uri, "POST", query_params, header_params, form_params, data,
      content_type, [callback](client::HttpResponse http_response) {
        if (http_response.status != 200) {
          callback(IngestDataResponse{client::ApiError(
              http_response.status, http_response.response.str())});
          return;
        }

        callback(IngestDataResponse(olp::parser::parse<model::ResponseOkSingle>(
            http_response.response)));
      });

  return cancel_token;
}

IngestDataResponse IngestApi::IngestData(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& content_type,
    const std::shared_ptr<std::vector<unsigned char>>& data,
    const boost::optional<std::string>& trace_id,
    const boost::optional<std::string>& billing_tag,
    const boost::optional<std::string>& checksum,
    client::CancellationContext context) {
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;
  std::multimap<std::string, std::string> header_params;

  header_params.insert(std::make_pair("Accept", "application/json"));
  if (trace_id) {
    header_params.insert(std::make_pair(kHeaderParamTraceId, trace_id.get()));
  }
  if (checksum) {
    header_params.insert(std::make_pair(kHeaderParamChecksum, checksum.get()));
  }

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  const std::string ingest_uri = "/layers/" + layer_id;

  auto http_response = client.CallApi(
      ingest_uri, "POST", std::move(query_params), std::move(header_params),
      std::move(form_params), data, content_type, context);
  if (http_response.status != olp::http::HttpStatusCode::OK) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Error during OlpClient::CallApi call, uri=%s, status=%i",
        ingest_uri.c_str(), http_response.status);
    return IngestDataResponse{
        client::ApiError(http_response.status, http_response.response.str())};
  }

  return IngestDataResponse(
      olp::parser::parse<model::ResponseOkSingle>(http_response.response));
}

IngestSdiiResponse IngestApi::IngestSdii(
    const client::OlpClient& client, const std::string& layer_id,
    const std::shared_ptr<std::vector<unsigned char>>& sdii_message_list,
    const boost::optional<std::string>& trace_id,
    const boost::optional<std::string>& billing_tag,
    const boost::optional<std::string>& checksum,
    client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));
  if (trace_id) {
    header_params.insert(std::make_pair(kHeaderParamTraceId, trace_id.get()));
  }
  if (checksum) {
    header_params.insert(std::make_pair(kHeaderParamChecksum, checksum.get()));
  }

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string ingest_uri = "/layers/" + layer_id + "/sdiiMessageList";
  auto response = client.CallApi(ingest_uri, "POST", query_params,
                                 header_params, form_params, sdii_message_list,
                                 "application/x-protobuf", context);

  if (response.status != olp::http::HttpStatusCode::OK) {
    return IngestSdiiResponse(
        ApiError(response.status, response.response.str()));
  }

  return IngestSdiiResponse(
      olp::parser::parse<model::ResponseOk>(response.response));
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
