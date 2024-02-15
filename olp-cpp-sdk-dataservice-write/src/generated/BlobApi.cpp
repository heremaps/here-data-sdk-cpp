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

#include "BlobApi.h"

#include <map>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/http/HttpStatusCode.h>

namespace client = olp::client;

namespace {
const std::string kQueryParamBillingTag = "billingTag";
}  // namespace

namespace olp {
namespace dataservice {
namespace write {

client::CancellationToken BlobApi::PutBlob(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& content_type, const std::string& content_encoding,
    const std::string& data_handle,
    const std::shared_ptr<std::vector<unsigned char>>& data,
    const boost::optional<std::string>& billing_tag, PutBlobCallback callback) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));
  if (!content_encoding.empty()) {
    header_params.insert(std::make_pair("Content-Encoding", content_encoding));
  }

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string put_blob_uri = "/layers/" + layer_id + "/data/" + data_handle;

  auto cancel_token = client.CallApi(
      put_blob_uri, "PUT", query_params, header_params, form_params, data,
      content_type, [callback](client::HttpResponse http_response) {
        if (http_response.status != http::HttpStatusCode::OK &&
            http_response.status != http::HttpStatusCode::NO_CONTENT) {
          callback(PutBlobResponse(client::ApiError(
              http_response.status, http_response.response.str())));
          return;
        }

        callback(PutBlobResponse(client::ApiNoResult()));
      });

  return cancel_token;
}

PutBlobResponse BlobApi::PutBlob(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& content_type, const std::string& content_encoding,
    const std::string& data_handle,
    const std::shared_ptr<std::vector<unsigned char>>& data,
    const boost::optional<std::string>& billing_tag,
    client::CancellationContext cancel_context) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));

  if (!content_encoding.empty()) {
    header_params.insert(std::make_pair("Content-Encoding", content_encoding));
  }

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string put_blob_uri = "/layers/" + layer_id + "/data/" + data_handle;

  auto http_response =
      client.CallApi(std::move(put_blob_uri), "PUT", std::move(query_params),
                     std::move(header_params), std::move(form_params), data,
                     content_type, cancel_context);

  if (http_response.status != olp::http::HttpStatusCode::OK &&
      http_response.status != olp::http::HttpStatusCode::NO_CONTENT) {
    return PutBlobResponse(
        client::ApiError(http_response.status, http_response.response.str()));
  }

  return PutBlobResponse(client::ApiNoResult());
}

client::CancellationToken BlobApi::deleteBlob(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& data_handle,
    const boost::optional<std::string>& billing_tag,
    const DeleteBlobCallback& callback) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string delete_blob_uri = "/layers/" + layer_id + "/data/" + data_handle;
  auto cancel_token = client.CallApi(
      delete_blob_uri, "DELETE", query_params, header_params, form_params,
      nullptr, "", [callback](client::HttpResponse http_response) {
        if (http_response.status != http::HttpStatusCode::OK &&
            http_response.status != http::HttpStatusCode::ACCEPTED) {
          callback(PutBlobResponse(client::ApiError(
              http_response.status, http_response.response.str())));
          return;
        }
        callback(DeleteBlobRespone(client::ApiNoResult()));
      });
  return cancel_token;
}

client::CancellationToken BlobApi::checkBlobExists(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& data_handle,
    const boost::optional<std::string>& billing_tag,
    const CheckBlobCallback& callback) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string check_blob_uri = "/layers/" + layer_id + "/data/" + data_handle;
  auto cancel_token = client.CallApi(
      check_blob_uri, "HEAD", query_params, header_params, form_params, nullptr,
      "", [callback](client::HttpResponse http_response) {
        if (http_response.status == http::HttpStatusCode::OK ||
            http_response.status == http::HttpStatusCode::NOT_FOUND) {
          callback(CheckBlobRespone(http_response.status));
          return;
        } else {
          callback(CheckBlobRespone(client::ApiError(
              http_response.status, http_response.response.str())));
        }
      });
  return cancel_token;
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
