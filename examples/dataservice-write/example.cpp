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

#include "example.h"

#include <fstream>
#include <iostream>

#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/HRN.h>
#include <olp/core/logging/Log.h>

#include <olp/dataservice/write/StreamLayerClient.h>
#include <olp/dataservice/write/model/PublishDataRequest.h>

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

namespace {
const std::string kKeyId("");            // your here.access.key.id
const std::string kKeySecret("");        // your here.access.key.secret
const std::string kCatalogHRN("");       // your catalog HRN where to write to
const std::string kLayer("");            // layer name inside catalog to use
const std::string kData("hello world");  // data to write

constexpr auto kLogTag = "write-example";
}  // namespace

int RunExample() {
  auto buffer = std::make_shared<std::vector<unsigned char>>(std::begin(kData),
                                                             std::end(kData));

  // Setup AuthenticationSettings with a default token provider that will
  // retrieve an OAuth 2.0 token from OLP.
  olp::client::AuthenticationSettings auth_settings;
  auth_settings.provider =
      olp::authentication::TokenProviderDefault(kKeyId, kKeySecret);

  // Setup OlpClientSettings and provide it to the StreamLayerClient.
  olp::client::OlpClientSettings client_settings;
  client_settings.authentication_settings = auth_settings;

  auto client = std::make_shared<StreamLayerClient>(
      olp::client::HRN{kCatalogHRN}, client_settings);

  // Create a publish data request
  auto request = PublishDataRequest().WithData(buffer).WithLayerId(kLayer);

  // Write data to OLP Stream Layer using StreamLayerClient
  auto future_response = client->PublishData(request);

  // Wait for response
  auto response = future_response.GetFuture().get();

  // Check the response
  if (!response.IsSuccessful()) {
    EDGE_SDK_LOG_ERROR_F(kLogTag,
                         "Error writing data - HTTP Status: %d Message: %s",
                         response.GetError().GetHttpStatusCode(),
                         response.GetError().GetMessage().c_str());
    return -1;
  } else {
    EDGE_SDK_LOG_INFO_F(kLogTag, "Publish Successful - TraceID: %s",
                        response.GetResult().GetTraceID().c_str());
  }

  return 0;
}
