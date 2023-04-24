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
using NetworkSettings = olp::http::NetworkSettings;

using TimeoutTest = NetworkTestBase;

TEST_F(TimeoutTest, TransferTimeout) {
  const std::string kUrlBase = "https://some-url.com";
  const std::string kApiBase = "/some-api";
  constexpr auto kTimeout = 1;

  // delay needed to simulate timeout
  mock_server_client_->MockResponse(
      "GET", kApiBase, mockserver::ReadDefaultResponses::GenerateData(), 200,
      true, kTimeout * 1000);

  settings_.WithTransferTimeout(std::chrono::seconds(kTimeout));
  const auto url = kUrlBase + kApiBase;
  const auto request = NetworkRequest(url).WithSettings(settings_).WithVerb(
      olp::http::NetworkRequest::HttpVerb::GET);

  std::promise<NetworkResponse> promise;
  const auto outcome =
      network_->Send(request, nullptr, [&promise](NetworkResponse response) {
        promise.set_value(std::move(response));
      });
  ASSERT_TRUE(outcome.IsSuccessful());

  auto future = promise.get_future();
  ASSERT_EQ(future.wait_for(std::chrono::seconds(2 * kTimeout)),
            std::future_status::ready);
  const auto response = future.get();

  EXPECT_EQ(response.GetStatus(),
            static_cast<int>(olp::http::ErrorCode::TIMEOUT_ERROR));
}

}  // namespace
