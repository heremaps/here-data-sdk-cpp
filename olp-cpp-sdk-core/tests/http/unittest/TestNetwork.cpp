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

#include <gtest/gtest.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>

TEST(TestNetwork, get_request) {
  auto network = olp::http::CreateDefaultNetwork(1);

  auto payload = std::make_shared<std::stringstream>();

  std::mutex m;
  std::condition_variable cv;

  olp::http::NetworkRequest request("http://localhost:3000/get_request");

  auto outcome = network->Send(
      request, payload,
      [payload, &cv](const olp::http::NetworkResponse& response) {
        EXPECT_EQ(response.GetStatus(), olp::http::HttpStatusCode::OK);
        EXPECT_EQ(payload->str(), "GET handler");
        cv.notify_one();
      });

  ASSERT_TRUE(outcome.IsSuccessfull());

  std::unique_lock<std::mutex> lock(m);
  ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(1)) ==
              std::cv_status::no_timeout);

  // At this moment there must be only 1 pointer to the network
  ASSERT_EQ(network.use_count(), 1);
}

TEST(TestNetwork, error_not_found) {
  auto network = olp::http::CreateDefaultNetwork(1);

  auto payload = std::make_shared<std::stringstream>();

  std::mutex m;
  std::condition_variable cv;

  olp::http::NetworkRequest request("http://localhost:3000/error_404");

  auto outcome = network->Send(
      request, payload,
      [payload, &cv](const olp::http::NetworkResponse& response) {
        EXPECT_EQ(response.GetStatus(), olp::http::HttpStatusCode::NOT_FOUND);
        cv.notify_one();
      });

  ASSERT_TRUE(outcome.IsSuccessfull());

  std::unique_lock<std::mutex> lock(m);
  ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(1)) ==
              std::cv_status::no_timeout);

  // At this moment there must be only 1 pointer to the network
  ASSERT_EQ(network.use_count(), 1);
}

TEST(TestNetwork, cancel_request) {
  auto network = olp::http::CreateDefaultNetwork(1);

  auto payload = std::make_shared<std::stringstream>();

  std::mutex m;
  std::condition_variable cv;

  olp::http::NetworkRequest request("http://localhost:3000/long_delay");

  auto outcome = network->Send(
      request, payload,
      [payload, &cv](const olp::http::NetworkResponse& response) {
        EXPECT_EQ(response.GetStatus(),
                  static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR));
        cv.notify_one();
      });

  ASSERT_TRUE(outcome.IsSuccessfull());

  network->Cancel(outcome.GetRequestId());

  std::unique_lock<std::mutex> lock(m);
  ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(1)) ==
              std::cv_status::no_timeout);

  // At this moment there must be only 1 pointer to the network
  ASSERT_EQ(network.use_count(), 1);
}

TEST(TestNetwork, reset_produce_offline_error) {
  auto network = olp::http::CreateDefaultNetwork(1);

  bool callback_called = false;

  olp::http::NetworkRequest request("http://localhost:3000/long_delay");

  auto outcome = network->Send(
      request, nullptr,
      [&callback_called](const olp::http::NetworkResponse& response) {
        EXPECT_EQ(response.GetStatus(),
                  static_cast<int>(olp::http::ErrorCode::OFFLINE_ERROR));
        EXPECT_EQ(response.GetError(), "Offline: network is deinitialized");
        callback_called = true;
      });

  ASSERT_TRUE(outcome.IsSuccessfull());

  // reseting the pointer must immediately trigger all request callbacks with
  // offline error.
  network = nullptr;
  ASSERT_TRUE(callback_called);
}

TEST(TestNetwork, post_request) {
  auto network = olp::http::CreateDefaultNetwork(1);

  const char* body_string = "Echo server";

  auto body = std::make_shared<std::vector<std::uint8_t>>();
  std::copy(body_string, body_string + strlen(body_string),
            std::back_inserter(*body));

  olp::http::NetworkRequest request("http://localhost:3000/echo");
  request.WithVerb(olp::http::NetworkRequest::HttpVerb::POST)
      .WithBody(std::move(body));

  std::mutex m;
  std::condition_variable cv;

  auto payload = std::make_shared<std::stringstream>();

  auto outcome = network->Send(
      request, payload,
      [&cv, payload, body_string](const olp::http::NetworkResponse& response) {
        EXPECT_EQ(payload->str(), body_string);
        EXPECT_EQ(response.GetStatus(), olp::http::HttpStatusCode::OK);
        cv.notify_one();
      });

  ASSERT_TRUE(outcome.IsSuccessfull());

  std::unique_lock<std::mutex> lock(m);
  ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(1)) ==
              std::cv_status::no_timeout);
}

TEST(TestNetwork, DISABLED_parallel_requests_limit) {
  const auto kParallelRequests = 4;
  auto network = olp::http::CreateDefaultNetwork(kParallelRequests);

  std::atomic_int callback_calls{0};

  for (int i = 0; i < kParallelRequests + 1; ++i) {
    olp::http::NetworkRequest request("http://localhost:3000/long_delay");

    auto outcome = network->Send(
        request, nullptr,
        [&callback_calls](const olp::http::NetworkResponse& response) {
          EXPECT_EQ(response.GetStatus(),
                    static_cast<int>(olp::http::ErrorCode::OFFLINE_ERROR));
          EXPECT_EQ(response.GetError(), "Offline: network is deinitialized");
          callback_calls.fetch_add(1);
        });

    ASSERT_TRUE(i < kParallelRequests && outcome.IsSuccessfull());
    ASSERT_TRUE(i == kParallelRequests && !outcome.IsSuccessfull() &&
                outcome.GetErrorCode() ==
                    olp::http::ErrorCode::NETWORK_OVERLOAD_ERROR);
  }

  // reseting the pointer must immediately trigger all request callbacks with
  // offline error.
  network = nullptr;
  EXPECT_EQ(callback_calls.load(), kParallelRequests);
}

TEST(TestNetwork, test_proxy) {
  auto network = olp::http::CreateDefaultNetwork(1);

  std::mutex m;
  std::condition_variable cv;

  olp::http::NetworkRequest request("http://platform.here.com");

  olp::http::NetworkSettings settings;
  settings.WithProxySettings(
      olp::http::NetworkProxySettings()
          .WithHostname("http://localhost:3000/http_proxy")
          .WithUsername("test_user")
          .WithPassword("test_password")
          .WithType(olp::http::NetworkProxySettings::Type::HTTP));

  request.WithSettings(settings);

  auto payload = std::make_shared<std::stringstream>();

  auto outcome = network->Send(
      request, payload,
      [&cv, payload](const olp::http::NetworkResponse& response) {
        EXPECT_EQ(response.GetStatus(),
                  static_cast<int>(olp::http::HttpStatusCode::OK));
        EXPECT_EQ(payload->str(), "Success");
        cv.notify_one();
      });

  EXPECT_TRUE(outcome.IsSuccessfull());

  std::unique_lock<std::mutex> lock(m);

  ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(1)) ==
              std::cv_status::no_timeout);
}
