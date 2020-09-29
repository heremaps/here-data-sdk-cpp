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

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/HttpStatusCode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mocks/NetworkMock.h>
#include <future>

using olp::client::OlpClientSettings;
using olp::client::OlpClientSettingsFactory;
using ::testing::_;
namespace {

TEST(OlpClientSettingsFactoryTest, PrewarmConnection) {
  OlpClientSettings settings;
  auto network = std::make_shared<NetworkMock>();
  settings.network_request_handler = network;
  EXPECT_CALL(*network, Send(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly(
          [&](olp::http::NetworkRequest /*request*/,
              olp::http::Network::Payload /*payload*/,
              olp::http::Network::Callback callback,
              olp::http::Network::HeaderCallback /*header_callback*/,
              olp::http::Network::DataCallback /*data_callback*/) {
            callback(olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::OK));
            return olp::http::SendOutcome(olp::http::RequestId(5));
          });

  const auto promise = std::make_shared<std::promise<int> >();
  olp::http::Network::Callback cb = [=](olp::http::NetworkResponse response) {
    promise->set_value(response.GetStatus());
  };
  // call with user callback
  OlpClientSettingsFactory::PrewarmConnection(settings, "url", cb);
  auto future = promise->get_future();
  ASSERT_EQ(std::future_status::ready,
            future.wait_for(std::chrono::seconds(1)));
  EXPECT_EQ(future.get(), olp::http::HttpStatusCode::OK);
  // call without callback
  OlpClientSettingsFactory::PrewarmConnection(settings, "url");
}
}  // namespace
