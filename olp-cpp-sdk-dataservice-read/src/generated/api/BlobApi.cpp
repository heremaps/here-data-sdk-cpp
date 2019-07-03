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

#include <map>

#include <olp/core/client/OlpClient.h>
#include <olp/core/network/HttpResponse.h>

namespace olp {
namespace dataservice {
namespace read {
using namespace olp::client;

CancellationToken BlobApi::GetBlob(
    const OlpClient& client, const std::string& layerId,
    const std::string& dataHandle, boost::optional<std::string> billingTag,
    boost::optional<std::string> range,
    const DataResponseCallback& dataResponseCallback) {
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("Accept", "application/json"));
  if (range) {
    headerParams.insert(std::make_pair("Range", *range));
  }

  std::multimap<std::string, std::string> queryParams;
  if (billingTag) {
    queryParams.insert(std::make_pair("billingTag", *billingTag));
  }

  std::multimap<std::string, std::string> formParams;

  std::string metadataUri = "/layers/" + layerId + "/data/" + dataHandle;

  NetworkAsyncCallback callback =
      [dataResponseCallback](network::HttpResponse response) {
        if (response.status != 200) {
          dataResponseCallback(ApiError(response.status, response.response));
        } else {
          dataResponseCallback(
              // TODO: response from HttpResponse should be already in
              // raw data format to avoid copy
              std::make_shared<std::vector<unsigned char>>(
                  response.response.begin(), response.response.end()));
        }
      };

  return client.CallApi(metadataUri, "GET", queryParams, headerParams,
                        formParams, nullptr, "", callback);
}

}  // namespace read

}  // namespace dataservice

}  // namespace olp
