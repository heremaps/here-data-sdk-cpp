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

#include "IndexApi.h"

#include <memory>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
// clang-format off
// Ordering Required - Parser template specializations before "JsonResultParser.h"
#include <generated/serializer/UpdateIndexRequestSerializer.h>
#include <generated/serializer/IndexInfoSerializer.h>
#include <generated/serializer/JsonSerializer.h>
// clang-format on

namespace {
const std::string kQueryParamBillingTag = "billingTag";
}  // namespace

namespace olp {
namespace dataservice {
namespace write {

client::CancellableFuture<InsertIndexesResponse> IndexApi::insertIndexes(
    const client::OlpClient& client, const model::Index& indexes,
    const std::string& layer_id,
    const boost::optional<std::string>& billing_tag) {
  auto promise = std::make_shared<std::promise<InsertIndexesResponse>>();
  auto cancel_token = insertIndexes(client, indexes, layer_id, billing_tag,
                                    [promise](InsertIndexesResponse response) {
                                      promise->set_value(std::move(response));
                                    });
  return client::CancellableFuture<InsertIndexesResponse>(cancel_token,
                                                          promise);
}

client::CancellationToken IndexApi::insertIndexes(
    const client::OlpClient& client, const model::Index& indexes,
    const std::string& layer_id,
    const boost::optional<std::string>& billing_tag,
    InsertIndexesCallback callback) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string insert_indexes_uri = "/layers/" + layer_id;

  auto serialized_indexes = serializer::serialize(indexes);
  auto data = std::make_shared<std::vector<unsigned char>>(
      serialized_indexes.begin(), serialized_indexes.end());

  auto cancel_token = client.CallApi(
      insert_indexes_uri, "POST", query_params, header_params, form_params,
      data, "application/json", [callback](client::HttpResponse http_response) {
        if (http_response.GetStatus() > http::HttpStatusCode::CREATED) {
          callback(InsertIndexesResponse(client::ApiError(
              http_response.GetStatus(), http_response.GetResponseAsString())));
          return;
        }

        callback(InsertIndexesResponse(client::ApiNoResult()));
      });
  return cancel_token;
}

InsertIndexesResponse IndexApi::InsertIndexes(
    const client::OlpClient& client, const model::Index& indexes,
    const std::string& layer_id,
    const boost::optional<std::string>& billing_tag,
    client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string insert_indexes_uri = "/layers/" + layer_id;

  auto serialized_indexes = serializer::serialize(indexes);
  auto data = std::make_shared<std::vector<unsigned char>>(
      serialized_indexes.begin(), serialized_indexes.end());

  auto http_response =
      client.CallApi(insert_indexes_uri, "POST", query_params, header_params,
                     form_params, data, "application/json", context);
  if (http_response.GetStatus() > http::HttpStatusCode::CREATED) {
    return client::ApiError(http_response.GetStatus(), http_response.GetResponseAsString());
  }

  return client::ApiNoResult();
}

client::CancellableFuture<UpdateIndexesResponse> IndexApi::performUpdate(
    const client::OlpClient& client, const model::UpdateIndexRequest& request,
    const boost::optional<std::string>& billing_tag) {
  auto promise = std::make_shared<std::promise<UpdateIndexesResponse>>();
  auto cancel_token = performUpdate(client, request, billing_tag,
                                    [promise](InsertIndexesResponse response) {
                                      promise->set_value(std::move(response));
                                    });
  return client::CancellableFuture<InsertIndexesResponse>(cancel_token,
                                                          promise);
}

client::CancellationToken IndexApi::performUpdate(
    const client::OlpClient& client, const model::UpdateIndexRequest& request,
    const boost::optional<std::string>& billing_tag,
    InsertIndexesCallback callback) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string update_indexes_uri = "/layers/" + request.GetLayerId();

  auto serialized_update_request = serializer::serialize(request);
  auto data = std::make_shared<std::vector<unsigned char>>(
      serialized_update_request.begin(), serialized_update_request.end());

  auto cancel_token = client.CallApi(
      update_indexes_uri, "PUT", query_params, header_params, form_params, data,
      "application/json", [callback](client::HttpResponse http_response) {
        if (http_response.GetStatus() > http::HttpStatusCode::CREATED) {
          callback(InsertIndexesResponse(client::ApiError(
              http_response.GetStatus(), http_response.GetResponseAsString())));
          return;
        }

        callback(InsertIndexesResponse(client::ApiNoResult()));
      });
  return cancel_token;
}
}  // namespace write
}  // namespace dataservice
}  // namespace olp
