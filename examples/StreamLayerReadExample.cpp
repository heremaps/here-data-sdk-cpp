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

#include "StreamLayerReadExample.h"

#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/utils/Base64.h>
#include <olp/dataservice/read/StreamLayerClient.h>

#include <future>
#include <iostream>

namespace {
constexpr auto kLogTag = "read-stream-layer-example";
}  // namespace

int RunStreamLayerExampleRead(const AccessKey& access_key,
                              const std::string& catalog,
                              const std::string& layer_id) {
  // Create a task scheduler instance
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  // Create a network client
  std::shared_ptr<olp::http::Network> http_client = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

  // Initialize authentication settings
  olp::authentication::Settings settings({access_key.id, access_key.secret});
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

  // Create stream layer client with settings and catalog, layer specified
  olp::dataservice::read::StreamLayerClient client(olp::client::HRN{catalog},
                                                   layer_id, client_settings);

  // Create subscription, used kSerial subscription mode, for reading smaller
  // volumes of data with a single subscription
  olp::dataservice::read::SubscribeRequest subscribe_request;
  subscribe_request.WithSubscriptionMode(
      olp::dataservice::read::SubscribeRequest::SubscriptionMode::kSerial);
  auto subscribe_future = client.Subscribe(subscribe_request);
  auto subscribe_response = subscribe_future.GetFuture().get();
  if (!subscribe_response.IsSuccessful()) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Failed to create subscription - HTTP Status: %d Message: %s",
        subscribe_response.GetError().GetHttpStatusCode(),
        subscribe_response.GetError().GetMessage().c_str());
    return -1;
  }
  unsigned int messages_size = 0;
  // Get the messages, and commit offsets till all data is consumed
  do {
    auto poll_future = client.Poll();
    auto poll_response = poll_future.GetFuture().get();
    if (!poll_response.IsSuccessful()) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Failed to poll data - HTTP Status: %d Message: %s",
                          poll_response.GetError().GetHttpStatusCode(),
                          poll_response.GetError().GetMessage().c_str());
      return -1;
    }

    auto result = poll_response.MoveResult();
    const auto& messages = result.GetMessages();
    messages_size = messages.size();
    if (messages_size > 0) {
      OLP_SDK_LOG_INFO_F(kLogTag, "Poll data - Success, messages size - %lu",
                         messages.size());
      for (const auto& msg : messages) {
        auto handle = msg.GetMetaData().GetDataHandle();
        if (handle) {
          OLP_SDK_LOG_INFO_F(kLogTag, "Message data: handle - %s, size - %lu",
                             handle.get().c_str(),
                             msg.GetMetaData().GetDataSize().get());
          // use GetData(const model::Message& message) with message instance to
          // request actual data
          auto message_future = client.GetData(msg);
          auto message_result = message_future.GetFuture().get();
          if (!message_result.IsSuccessful()) {
            OLP_SDK_LOG_ERROR_F(kLogTag,
                                "Failed to get data for data handle %s - HTTP "
                                "Status: %d Message: %s",
                                handle.get().c_str(),
                                message_result.GetError().GetHttpStatusCode(),
                                message_result.GetError().GetMessage().c_str());
            return -1;
          } else {
            auto message_data = message_result.MoveResult();
            OLP_SDK_LOG_INFO_F(kLogTag, "GetData for %s successful: size - %lu",
                               handle.get().c_str(), message_data->size());
          }
        } else {
          OLP_SDK_LOG_INFO_F(kLogTag, "Message data: size - %lu",
                             msg.GetData()->size());
        }
      }
    } else {
      OLP_SDK_LOG_INFO(kLogTag, "No new messages is received");
    }
  } while (messages_size > 0);
  return 0;
}
