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

#include "BlobApi.h"
#include <olp/core/network/HttpResponse.h>

using namespace olp::client;

namespace {
const std::string kQueryParamBillingTag = "billingTag";
}  // namespace

namespace olp {
namespace dataservice {
namespace write {

CancellableFuture<PutBlobResponse> BlobApi::PutBlob(
    const OlpClient& client, const std::string& layer_id,
    const std::string& content_type, const std::string& data_handle,
    const std::shared_ptr<std::vector<unsigned char>>& data,
    const boost::optional<std::string>& billing_tag) {
  auto promise = std::make_shared<std::promise<PutBlobResponse>>();
  auto cancel_token = PutBlob(client, layer_id, content_type, data_handle, data,
                              billing_tag, [promise](PutBlobResponse response) {
                                promise->set_value(std::move(response));
                              });
  return CancellableFuture<PutBlobResponse>(cancel_token, promise);
}

CancellationToken BlobApi::PutBlob(
    const OlpClient& client, const std::string& layer_id,
    const std::string& content_type, const std::string& data_handle,
    const std::shared_ptr<std::vector<unsigned char>>& data,
    const boost::optional<std::string>& billing_tag, PutBlobCallback callback) {
  std::multimap<std::string, std::string> header_params;
  std::multimap<std::string, std::string> query_params;
  std::multimap<std::string, std::string> form_params;

  header_params.insert(std::make_pair("Accept", "application/json"));

  if (billing_tag) {
    query_params.insert(
        std::make_pair(kQueryParamBillingTag, billing_tag.get()));
  }

  std::string put_blob_uri = "/layers/" + layer_id + "/data/" + data_handle;

  auto cancel_token = client.CallApi(
      put_blob_uri, "PUT", query_params, header_params, form_params, data,
      content_type, [callback](network::HttpResponse http_response) {
        if (http_response.status != 200) {
          callback(PutBlobResponse(
              ApiError(http_response.status, http_response.response)));
          return;
        }

        callback(PutBlobResponse(ApiNoResult()));
      });

  return cancel_token;
}

CancellationToken BlobApi::deleteBlob(
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
      nullptr, "", [callback](network::HttpResponse http_response) {
        if (http_response.status != 200 && http_response.status != 202) {
          callback(PutBlobResponse(
              ApiError(http_response.status, http_response.response)));
          return;
        }
        callback(DeleteBlobRespone(ApiNoResult()));
      });
  return cancel_token;
}

CancellationToken BlobApi::checkBlobExists(
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
      "", [callback](network::HttpResponse http_response) {
        if (http_response.status == 200 || http_response.status == 404) {
          callback(CheckBlobRespone(http_response.status));
          return;
        } else {
          callback(CheckBlobRespone(
              ApiError(http_response.status, http_response.response)));
        }
      });
  return cancel_token;
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
