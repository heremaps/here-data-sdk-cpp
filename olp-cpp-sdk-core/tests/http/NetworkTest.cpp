/*
 * Copyright (C) 2026 HERE Europe B.V.
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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <memory>
#include <thread>

#include <olp/core/http/Network.h>

#include "LoopbackHttpServer.h"

namespace {

using namespace olp::http;

TEST(NetworkLifetimeTest,
     CallbackReleasesNetworkObjectShouldStayAliveDuringCallback) {
  auto network = CreateDefaultNetwork({});

  std::promise<void> promise;
  auto future = promise.get_future();

  NetworkRequest req(test::CreateLoopbackUrl());

  auto callback = [&network, &promise](NetworkResponse /*resp*/) {
    network.reset();
    promise.set_value();
  };

  auto outcome = network->Send(req, nullptr, callback, nullptr, nullptr);
  ASSERT_TRUE(outcome.IsSuccessful()) << "Send failed before test could run";

  // Wait for callback to be executed (timeout to avoid hangs in CI).
  auto status = future.wait_for(std::chrono::seconds(1));
  ASSERT_EQ(status, std::future_status::ready)
      << "Callback did not run in time";
}

TEST(NetworkLifetimeTest, NoCrashIfNetworkDestroyedDuringCallback) {
  auto network = CreateDefaultNetwork({});

  std::promise<void> promise;
  auto future = promise.get_future();

  NetworkRequest req(test::CreateLoopbackUrl());

  auto callback = [&network, &promise](NetworkResponse /*resp*/) {
    // Make network destroy async, from another thread.
    std::thread([&network, &promise]() {
      network.reset();
      promise.set_value();
    }).detach();
  };

  auto outcome = network->Send(req, nullptr, callback, nullptr, nullptr);
  ASSERT_TRUE(outcome.IsSuccessful()) << "Send failed before test could run";

  // Wait for callback to be executed (timeout to avoid hangs in CI).
  auto status = future.wait_for(std::chrono::seconds(2));
  ASSERT_EQ(status, std::future_status::ready)
      << "Callback did not run in time";
}

TEST(NetworkLifetimeTest, NoCrashIfNetworkDestroyedDuringHeaderCallback) {
  auto network = CreateDefaultNetwork({});

  std::promise<void> promise;
  auto future = promise.get_future();
  auto fired = std::make_shared<std::atomic<bool>>(false);

  NetworkRequest req(test::CreateLoopbackUrl());

  auto header_callback = [&network, &promise, fired](std::string /*key*/,
                                                     std::string /*value*/) {
    if (fired->exchange(true)) {
      return;
    }
    // Make network destroy async, from another thread.
    std::thread([&network, &promise]() {
      network.reset();
      promise.set_value();
    }).detach();
  };

  auto outcome = network->Send(
      req, nullptr, [](NetworkResponse /*resp*/) {}, header_callback, nullptr);
  ASSERT_TRUE(outcome.IsSuccessful()) << "Send failed before test could run";

  // Wait for callback to be executed (timeout to avoid hangs in CI).
  auto status = future.wait_for(std::chrono::seconds(2));
  ASSERT_EQ(status, std::future_status::ready)
      << "Callback did not run in time";
}

TEST(NetworkLifetimeTest, NoCrashIfNetworkDestroyedDuringDataCallback) {
  auto network = CreateDefaultNetwork({});

  std::promise<void> promise;
  auto future = promise.get_future();
  auto fired = std::make_shared<std::atomic<bool>>(false);

  NetworkRequest req(test::CreateLoopbackUrl());

  auto data_callback = [&network, &promise, fired](const std::uint8_t* /*data*/,
                                                   std::uint64_t /*offset*/,
                                                   std::size_t /*length*/) {
    if (fired->exchange(true)) {
      return;
    }
    // Make network destroy async, from another thread.
    std::thread([&network, &promise]() {
      network.reset();
      promise.set_value();
    }).detach();
  };

  auto outcome = network->Send(
      req, nullptr, [](NetworkResponse /*resp*/) {}, nullptr, data_callback);
  ASSERT_TRUE(outcome.IsSuccessful()) << "Send failed before test could run";

  // Wait for callback to be executed (timeout to avoid hangs in CI).
  auto status = future.wait_for(std::chrono::seconds(2));
  ASSERT_EQ(status, std::future_status::ready)
      << "Callback did not run in time";
}

TEST(NetworkLifetimeTest, NoCrashIfNetworkDestroyedImmediatelyAfterSend) {
  auto network = CreateDefaultNetwork({});

  NetworkRequest req(test::CreateLoopbackUrl());

  auto outcome = network->Send(
      req, nullptr, [](NetworkResponse /*resp*/) {},
      [](std::string /*key*/, std::string /*value*/) {},
      [](const std::uint8_t* /*data*/, std::uint64_t /*offset*/,
         std::size_t /*length*/) {});
  ASSERT_TRUE(outcome.IsSuccessful()) << "Send failed before test could run";

  // Destroy network immediately after send.
  network.reset();
}

}  // namespace
