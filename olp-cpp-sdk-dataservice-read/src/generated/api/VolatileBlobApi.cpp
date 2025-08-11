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

#include "VolatileBlobApi.h"

#include <map>
#include <memory>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>

namespace olp {
namespace dataservice {
namespace read {

VolatileBlobApi::DataResponse VolatileBlobApi::GetVolatileBlob(
    const client::OlpClient& client, const std::string& layer_id,
    const std::string& data_handle, porting::optional<std::string> billing_tag,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));
  std::multimap<std::string, std::string> query_params;
  if (billing_tag) {
    query_params.insert(std::make_pair("billingTag", *billing_tag));
  }

  std::multimap<std::string, std::string> form_params;
  std::string metadata_uri = "/layers/" + layer_id + "/data/" + data_handle;
  auto api_response =
      client.CallApi(metadata_uri, "GET", query_params, header_params,
                     form_params, nullptr, "", context);

  if (api_response.GetStatus() != http::HttpStatusCode::OK) {
    return DataResponse(client::ApiError(api_response.GetStatus(),
                                         api_response.GetResponseAsString()),
                        api_response.GetNetworkStatistics());
  }

  auto result = std::make_shared<std::vector<unsigned char>>();
  api_response.GetResponse(*result);
  return DataResponse(result, api_response.GetNetworkStatistics());
}
}  // namespace read
}  // namespace dataservice
}  // namespace olp
