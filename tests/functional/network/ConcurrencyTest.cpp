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
using RequestId = olp::http::RequestId;

const std::string kUrlBase = "https://some-url.com";
const std::string kApiBase = "/some-api/";
constexpr auto kTimeout = std::chrono::seconds(3);

class ConcurrencyTest : public NetworkTestBase {
 public:
  void AddExpectation(
      int i, olp::porting::optional<int32_t> delay_ms = olp::porting::none) {
    const auto url = kApiBase + std::to_string(i);
    mock_server_client_->MockResponse(
        "GET", url, mockserver::ReadDefaultResponses::GenerateData(), 200, true,
        delay_ms);
  }

  RequestId SendRequest(int i) {
    const auto url = kUrlBase + kApiBase + std::to_string(i);
    const auto request = NetworkRequest(url).WithSettings(settings_).WithVerb(
        olp::http::NetworkRequest::HttpVerb::GET);
    const auto outcome =
        network_->Send(request, nullptr,
                       std::bind(&ConcurrencyTest::ResponseCallback, this,
                                 std::placeholders::_1));

    EXPECT_TRUE(outcome.IsSuccessful());
    return outcome.GetRequestId();
  }

  void ResponseCallback(NetworkResponse response) {
    std::unique_lock<std::mutex> lock(result_mutex_);
    const auto id = response.GetRequestId();
    responses_.push_back(id);
    finish_cv_.notify_one();
  }

 protected:
  std::mutex result_mutex_;
  std::condition_variable finish_cv_;
  std::vector<RequestId> responses_;
};

TEST_F(ConcurrencyTest, ResponseDelay) {
  constexpr auto kRequestCount = 10;

  // setup network expectation
  // first and last requests are 500ms delayed
  AddExpectation(0, 500);
  for (int i = 1; i < kRequestCount - 1; ++i) {
    AddExpectation(i);
  }
  AddExpectation(kRequestCount - 1, 500);

  auto first_request_id = SendRequest(0);
  for (int i = 1; i < kRequestCount - 1; ++i) {
    SendRequest(i);
  }
  auto last_request_id = SendRequest(kRequestCount - 1);

  {
    std::unique_lock<std::mutex> lock(result_mutex_);
    ASSERT_TRUE(finish_cv_.wait_for(
        lock, kTimeout, [&]() { return responses_.size() == kRequestCount; }));
  }
  ASSERT_EQ(responses_.size(), kRequestCount);
  ASSERT_TRUE(responses_[kRequestCount - 2] == first_request_id ||
              responses_[kRequestCount - 2] == last_request_id);
  ASSERT_TRUE(responses_[kRequestCount - 1] == first_request_id ||
              responses_[kRequestCount - 1] == last_request_id);
}

}  // namespace
