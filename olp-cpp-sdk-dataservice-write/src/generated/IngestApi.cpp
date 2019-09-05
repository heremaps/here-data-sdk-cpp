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

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>

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
}  // namespace

namespace olp {
namespace dataservice {
namespace write {
olp::client::CancellableFuture<IngestDataResponse> IngestApi::IngestData(
    const OlpClient& client, const std::string& layer_id,
    const std::string& content_type,
    const std::shared_ptr<std::vector<unsigned char>>& data,
    const boost::optional<std::string>& trace_id,
    const boost::optional<std::string>& billing_tag,
    const boost::optional<std::string>& checksum) {
  auto promise = std::make_shared<std::promise<IngestDataResponse>>();
  auto cancel_token =
      IngestData(client, layer_id, content_type, data, trace_id, billing_tag,
                 checksum, [promise](IngestDataResponse response) {
                   promise->set_value(std::move(response));
                 });
  return client::CancellableFuture<IngestDataResponse>(cancel_token, promise);
}

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
          callback(IngestDataResponse{
              client::ApiError(http_response.status, http_response.response)});
          return;
        }

        callback(IngestDataResponse(olp::parser::parse<model::ResponseOkSingle>(
            http_response.response)));
      });

  return cancel_token;
}

olp::client::CancellableFuture<IngestSdiiResponse> IngestApi::IngestSDII(
    const OlpClient& client, const std::string& layer_id,
    const std::shared_ptr<std::vector<unsigned char>>& sdii_message_list,
    const boost::optional<std::string>& trace_id,
    const boost::optional<std::string>& billing_tag,
    const boost::optional<std::string>& checksum) {
  auto promise = std::make_shared<std::promise<IngestSdiiResponse>>();
  auto cancel_token =
      IngestSDII(client, layer_id, sdii_message_list, trace_id, billing_tag,
                 checksum, [promise](IngestSdiiResponse response) {
                   promise->set_value(std::move(response));
                 });
  return client::CancellableFuture<IngestSdiiResponse>(cancel_token, promise);
}

client::CancellationToken IngestApi::IngestSDII(
    const client::OlpClient& client, const std::string& layer_id,
    const std::shared_ptr<std::vector<unsigned char>>& sdii_message_list,
    const boost::optional<std::string>& trace_id,
    const boost::optional<std::string>& billing_tag,
    const boost::optional<std::string>& checksum, IngestSdiiCallback callback) {
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

  auto cancel_token = client.CallApi(
      ingest_uri, "POST", query_params, header_params, form_params,
      sdii_message_list, "application/x-protobuf",
      [callback](client::HttpResponse http_response) {
        if (http_response.status != 200) {
          callback(IngestSdiiResponse(
              ApiError(http_response.status, http_response.response)));
          return;
        }

        auto response_ok = std::make_shared<model::ResponseOk>(
            olp::parser::parse<model::ResponseOk>(http_response.response));

        callback(IngestSdiiResponse(
            olp::parser::parse<model::ResponseOk>(http_response.response)));
      });

  return cancel_token;
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
