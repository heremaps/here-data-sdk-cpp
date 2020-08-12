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

#include <memory>

#include <gtest/gtest.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkSettings.h>

#include "NetworkTestBase.h"
#include "ReadDefaultResponses.h"

namespace {
using NetworkRequest = olp::http::NetworkRequest;
using NetworkResponse = olp::http::NetworkResponse;

using DestructionTest = NetworkTestBase;

TEST_F(DestructionTest, Callback) {
  const std::string kUrlBase = "https://some-url.com";
  const std::string kApiBase = "/some-api/";
  constexpr auto kRequestCount = 10;

  for (int i = 0; i < kRequestCount; ++i) {
    const auto url = kApiBase + std::to_string(i);
    // delay needed to be sure network destroyed before any request is complete
    mock_server_client_->MockResponse(
        "GET", url, mockserver::ReadDefaultResponses::GenerateData(), 200, true,
        500);
  }

  std::vector<std::promise<NetworkResponse>> promises(kRequestCount);
  for (int i = 0; i < kRequestCount; ++i) {
    const auto url = kUrlBase + kApiBase + std::to_string(i);
    const auto request = NetworkRequest(url).WithSettings(settings_).WithVerb(
        olp::http::NetworkRequest::HttpVerb::GET);
    const auto outcome = network_->Send(
        request, nullptr, [&promises, i](NetworkResponse response) {
          promises[i].set_value(std::move(response));
        });

    ASSERT_TRUE(outcome.IsSuccessful());
  }

  mock_server_client_.reset();
  network_.reset();

  for (int i = 0; i < kRequestCount; ++i) {
    auto future = promises[i].get_future();
    const auto response = future.get();

    EXPECT_EQ(response.GetStatus(),
              static_cast<int>(olp::http::ErrorCode::OFFLINE_ERROR))
        << response.GetError();
  }
}

}  // namespace
