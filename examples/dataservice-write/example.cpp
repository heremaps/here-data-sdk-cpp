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
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
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
constexpr size_t kPublishRequestsSize = 5u;
}  // namespace

int RunExample() {
  auto buffer = std::make_shared<std::vector<unsigned char>>(std::begin(kData),
                                                             std::end(kData));

  // Create a task scheduler instance
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

  // Create a network client
  std::shared_ptr<olp::http::Network> http_client = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

  // Initialize authentication settings
  olp::authentication::Settings settings({kKeyId, kKeySecret});
  settings.task_scheduler = std::move(task_scheduler);
  settings.network_request_handler = http_client;

  // Setup AuthenticationSettings with a default token provider that will
  // retrieve an OAuth 2.0 token from OLP.
  olp::client::AuthenticationSettings auth_settings;
  auth_settings.provider =
      olp::authentication::TokenProviderDefault(std::move(settings));

  // Setup OlpClientSettings and provide it to the StreamLayerClient.
  olp::client::OlpClientSettings client_settings;
  client_settings.authentication_settings = auth_settings;
  client_settings.network_request_handler = std::move(http_client);

  auto client = std::make_shared<StreamLayerClient>(
      olp::client::HRN{kCatalogHRN}, std::move(client_settings));

  // Create a publish data request
  auto request = PublishDataRequest().WithData(buffer).WithLayerId(kLayer);

  // Single publish to stream layer
  {
    // Write data to OLP Stream Layer using StreamLayerClient
    auto future_response = client->PublishData(request);

    // Wait for response
    auto response = future_response.GetFuture().get();

    // Check the response
    if (!response.IsSuccessful()) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Error writing data - HTTP Status: %d Message: %s",
                          response.GetError().GetHttpStatusCode(),
                          response.GetError().GetMessage().c_str());
      return -1;
    }

    OLP_SDK_LOG_INFO_F(kLogTag, "Publish Successful - TraceID: %s",
                       response.GetResult().GetTraceID().c_str());
  }

  // Multi-publish to stream layer
  {
    // Queue publish requests to be uploaded on Flush
    for (size_t idx = 0; idx < kPublishRequestsSize; ++idx) {
      auto status = client->Queue(request);
      if (status) {
        OLP_SDK_LOG_ERROR_F(kLogTag, "Queue failed - %s", status->c_str());
        return -1;
      }
    }

    // Flush and wait for uploading
    auto flush_request = FlushRequest();
    auto future_response = client->Flush(std::move(flush_request));
    auto responses = future_response.GetFuture().get();
    if (responses.empty()) {
      OLP_SDK_LOG_ERROR_F(kLogTag, "Error on Flush()");
      return -1;
    }

    // Check all publishes succeeded
    for (auto response : responses) {
      if (!response.IsSuccessful()) {
        OLP_SDK_LOG_ERROR_F(kLogTag,
                            "Error flushing data - HTTP Status: %d Message: %s",
                            response.GetError().GetHttpStatusCode(),
                            response.GetError().GetMessage().c_str());
        return -1;
      }

      OLP_SDK_LOG_INFO_F(kLogTag, "Flush Successful - TraceID: %s",
                         response.GetResult().GetTraceID().c_str());
    }
  }

  return 0;
}
