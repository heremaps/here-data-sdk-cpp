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
#include <thread>

TEST(NetworkTest, GetRequest) {
  auto network = olp::http::CreateDefaultNetwork(1);

  auto payload = std::make_shared<std::stringstream>();

  std::mutex m;
  std::condition_variable cv;

  auto main_thread_id = std::this_thread::get_id();

  olp::http::NetworkRequest request("http://localhost:3000/get_request");

  auto outcome = network->Send(
      request, payload,
      [payload, &cv,
       main_thread_id](const olp::http::NetworkResponse& response) {
        EXPECT_EQ(response.GetStatus(), olp::http::HttpStatusCode::OK);
        EXPECT_EQ(payload->str(), "GET handler");
        EXPECT_NE(main_thread_id, std::this_thread::get_id());
        cv.notify_one();
      });

  ASSERT_TRUE(outcome.IsSuccessful());

  std::unique_lock<std::mutex> lock(m);
  ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(1)) ==
              std::cv_status::no_timeout);

  // At this moment there must be only 1 pointer to the network
  ASSERT_EQ(network.use_count(), 1);
}
