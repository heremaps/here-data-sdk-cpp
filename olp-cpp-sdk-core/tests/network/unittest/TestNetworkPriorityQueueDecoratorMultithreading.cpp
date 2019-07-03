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

#include <olp/core/network/NetworkRequestPriorityQueueDecorator.h>
#include "../mock/NetworkProtocolMock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <future>
#include <memory>
#include <thread>
#include <vector>

using olp::network::Network;
using olp::network::NetworkConfig;
using olp::network::NetworkProtocol;
using olp::network::NetworkRequest;
using olp::network::NetworkRequestPriorityQueueDecorator;
using olp::network::test::NetworkProtocolMock;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::UnorderedElementsAreArray;

namespace {
const auto number_of_threads = 10;
const auto requests_per_thread = 250;
const auto number_of_requests = number_of_threads * requests_per_thread;
const auto request = NetworkRequest("test-url");

std::mutex send_ids_mutex, promises_mutex;
std::vector<int> send_ids;
std::vector<std::promise<void>> send_done_promises;

NetworkProtocol::ErrorCode send_and_count_ids(
    const NetworkRequest&, int id, const std::shared_ptr<std::ostream>&,
    std::shared_ptr<NetworkConfig>, Network::HeaderCallback,
    Network::DataCallback, Network::Callback) {
  {
    std::lock_guard<std::mutex> lock(send_ids_mutex);
    send_ids.push_back(id);
  }
  {
    std::lock_guard<std::mutex> lock(promises_mutex);
    send_done_promises[id].set_value();
  }
  return NetworkProtocol::ErrorNone;
}

class NetworkRequestPriorityQueueDecoratorMultiThreadingTest
    : public ::testing::Test {
 protected:
  NetworkRequestPriorityQueueDecoratorMultiThreadingTest() {
    send_ids.clear();
    send_done_promises = std::vector<std::promise<void>>(number_of_requests);
  }

  void waitForSetupThenStartTest() {
    for_each(begin(m_threadReadySignals), end(m_threadReadySignals),
             [](std::promise<void>& threadReadySignal) {
               threadReadySignal.get_future().wait();
             });

    m_startTestSignal.set_value();
  }

  std::promise<void> m_startTestSignal;
  std::shared_future<void> m_startTest{m_startTestSignal.get_future()};
  std::array<std::promise<void>, number_of_threads> m_threadReadySignals;
};
}  // namespace

TEST_F(NetworkRequestPriorityQueueDecoratorMultiThreadingTest,
       if_10_threads_send_each_250_requests_then_2500_requests_are_sent) {
  auto underlying_protocol =
      std::make_shared<::testing::NiceMock<NetworkProtocolMock>>();
  ON_CALL(*underlying_protocol, Ready()).WillByDefault(Return(true));
  ON_CALL(*underlying_protocol, Send(_, _, _, _, _, _, _))
      .WillByDefault(Invoke(send_and_count_ids));
  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  std::vector<std::future<void>> done;
  std::vector<std::future<void>> sent_done;
  sent_done.reserve(number_of_requests);
  transform(begin(send_done_promises), end(send_done_promises),
            back_inserter(sent_done),
            [](std::promise<void>& promise) { return promise.get_future(); });

  for (auto i = 0; i < number_of_threads; ++i) {
    auto asyncDone = std::async(std::launch::async, [this, i, &protocol] {
      m_threadReadySignals[i].set_value();
      m_startTest.wait();

      for (auto j = 0; j < requests_per_thread; ++j) {
        auto index = requests_per_thread * i + j;
        protocol.Send(request, index, nullptr, nullptr, nullptr, nullptr,
                      nullptr);
      }
    });

    done.push_back(std::move(asyncDone));
  }

  waitForSetupThenStartTest();

  // wait until test has_finished
  for_each(begin(done), end(done),
           [](const std::future<void>& end) { end.wait(); });
  for_each(begin(sent_done), end(sent_done),
           [](const std::future<void>& end) { end.wait(); });

  std::vector<int> expected_ids(requests_per_thread * number_of_threads, 0);
  for (auto i = 0; i < static_cast<int>(expected_ids.size()); ++i)
    expected_ids[i] = i;
  EXPECT_THAT(send_ids, UnorderedElementsAreArray(expected_ids));
}
