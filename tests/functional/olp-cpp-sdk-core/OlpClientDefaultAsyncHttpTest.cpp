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

#include <gtest/gtest.h>

#include <future>

#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>

#include <olp/core/http/Network.h>

namespace http = olp::http;

class OlpClientDefaultAsyncHttpTest : public ::testing::Test {
 protected:
  olp::client::OlpClientSettings client_settings_;
};

TEST_F(OlpClientDefaultAsyncHttpTest, GetGoogleWebsite) {
  client_settings_.network_request_handler = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

  olp::client::OlpClient client(client_settings_, "https://www.google.com");

  std::promise<olp::client::HttpResponse> p;
  olp::client::NetworkAsyncCallback callback =
      [&p](olp::client::HttpResponse response) {
        p.set_value(std::move(response));
      };

  auto cancel_token = client.CallApi(std::string(), std::string(),
                                     std::multimap<std::string, std::string>(),
                                     std::multimap<std::string, std::string>(),
                                     std::multimap<std::string, std::string>(),
                                     nullptr, std::string(), callback);

  auto response = p.get_future().get();
  ASSERT_EQ(http::HttpStatusCode::OK, response.GetStatus());
  ASSERT_LT(0u, response.GetResponseAsString().size());
}

TEST_F(OlpClientDefaultAsyncHttpTest, GetNonExistentWebsite) {
  std::promise<olp::client::HttpResponse> p;
  olp::client::NetworkAsyncCallback callback =
      [&p](olp::client::HttpResponse response) {
        p.set_value(std::move(response));
      };

  client_settings_.network_request_handler = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

  // RFC 2606. Use reserved domain name that nobody could register.
  olp::client::OlpClient client(client_settings_, "https://example.test");

  auto cancel_token = client.CallApi(std::string(), std::string(),
                                     std::multimap<std::string, std::string>(),
                                     std::multimap<std::string, std::string>(),
                                     std::multimap<std::string, std::string>(),
                                     nullptr, std::string(), callback);
  auto response = p.get_future().get();
  ASSERT_EQ(olp::http::ErrorCode::INVALID_URL_ERROR,
            static_cast<olp::http::ErrorCode>(response.GetStatus()));
}
