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
#include <olp/core/network/NetworkResponse.h>
#include "../mock/NetworkProtocolMock.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <future>
#include <memory>

using olp::network::Network;
using olp::network::NetworkConfig;
using olp::network::NetworkProtocol;
using olp::network::NetworkRequest;
using olp::network::NetworkRequestPriorityQueueDecorator;
using olp::network::NetworkResponse;
using olp::network::test::NetworkProtocolMock;

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {
class AccumulateSentIds : public NetworkProtocol {
 public:
  bool Initialize() override { return false; }
  void Deinitialize() override {}
  bool Initialized() const override { return false; }
  ErrorCode Send(const NetworkRequest&, int id,
                 const std::shared_ptr<std::ostream>&,
                 std::shared_ptr<NetworkConfig>, Network::HeaderCallback,
                 Network::DataCallback, Network::Callback callback) override {
    m_sent_ids.push_back(id);
    if (!m_callbacks.empty()) m_callbacks[id] = callback;
    m_send_done[id].set_value();
    return NetworkProtocol::ErrorNone;
  }
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Winconsistent-missing-override"
#endif
  MOCK_METHOD0(Ready, bool());
  MOCK_METHOD1(Cancel, bool(int));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
  size_t AmountPending() override { return 0u; }

  void wait_until_promises_are_satisfied() {
    for_each(begin(m_send_done), end(m_send_done),
             [](std::promise<void>& send_promise) {
               send_promise.get_future().wait();
             });
  }

  std::vector<int> m_sent_ids;
  std::vector<Network::Callback> m_callbacks;
  std::vector<std::promise<void> > m_send_done;
};
}  // namespace

TEST(TestNetworkPriorityQueueDecorator,
     if_queue_is_empty_then_ready_returns_true) {
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  EXPECT_TRUE(protocol.Ready());
}

TEST(TestNetworkPriorityQueueDecorator,
     if_queue_is_not_full_then_ready_returns_true) {
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  EXPECT_CALL(*underlying_protocol, Ready()).WillRepeatedly(Return(false));
  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  for (auto i = 0; i < 50; ++i) {
    protocol.Send(NetworkRequest(), 0, nullptr, nullptr, nullptr, nullptr,
                  nullptr);
  }

  EXPECT_TRUE(protocol.Ready());
}

TEST(TestNetworkPriorityQueueDecorator,
     if_queued_request_is_canceled_then_it_is_removed_from_queue) {
  const auto number_of_requests = 11;
  std::promise<void> request_is_passed_to_worker_thread_promise;
  auto request_is_passed_to_worker_thread_future =
      request_is_passed_to_worker_thread_promise.get_future();
  const auto underlying_protocol =
      std::make_shared<NiceMock<AccumulateSentIds> >();
  underlying_protocol->m_send_done =
      std::vector<std::promise<void> >(number_of_requests);
  EXPECT_CALL(*underlying_protocol, Ready())
      .WillOnce(Return(false))  // on start-up
      .WillOnce(Invoke([&request_is_passed_to_worker_thread_promise] {
        request_is_passed_to_worker_thread_promise.set_value();
        return false;
      }))                             // for first request
      .WillRepeatedly(Return(true));  // for all following requests
  EXPECT_CALL(*underlying_protocol, Cancel(_)).Times(0);
  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  const auto test_id = 0;
  protocol.Send(NetworkRequest(), test_id, nullptr, nullptr, nullptr, nullptr,
                nullptr);
  request_is_passed_to_worker_thread_future.wait();
  EXPECT_TRUE(protocol.Cancel(test_id));
  underlying_protocol->m_send_done[test_id].set_value();

  for (auto i = 1; i < number_of_requests; ++i) {
    protocol.Send(NetworkRequest(), test_id + i, nullptr, nullptr, nullptr,
                  nullptr, nullptr);
  }

  underlying_protocol->wait_until_promises_are_satisfied();

  const auto contains_test_id = any_of(begin(underlying_protocol->m_sent_ids),
                                       end(underlying_protocol->m_sent_ids),
                                       [&](int id) { return id == test_id; });
  EXPECT_FALSE(contains_test_id);
}

TEST(TestNetworkPriorityQueueDecorator,
     if_queue_is_not_full_then_send_returns_ErrorNone) {
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);
  const auto error_code = protocol.Send(NetworkRequest(), 0, nullptr, nullptr,
                                        nullptr, nullptr, nullptr);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));
}

TEST(TestNetworkPriorityQueueDecorator,
     if_decorated_protocol_is_not_ready_then_request_is_not_sent) {
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  EXPECT_CALL(*underlying_protocol, Ready()).WillRepeatedly(Return(false));
  EXPECT_CALL(*underlying_protocol, Send(_, _, _, _, _, _, _)).Times(0);

  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);
  const auto error_code = protocol.Send(NetworkRequest(), 0, nullptr, nullptr,
                                        nullptr, nullptr, nullptr);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));
}

TEST(TestNetworkPriorityQueueDecorator,
     when_decorated_protocol_becomes_ready_then_request_is_sent) {
  const auto number_of_requests = 2;
  std::promise<void> request_is_passed_to_worker_thread_promise;
  auto request_is_passed_to_worker_thread_future =
      request_is_passed_to_worker_thread_promise.get_future();
  const auto underlying_protocol =
      std::make_shared<NiceMock<AccumulateSentIds> >();
  underlying_protocol->m_send_done =
      std::vector<std::promise<void> >(number_of_requests);
  EXPECT_CALL(*underlying_protocol, Ready())
      .WillOnce(Return(false))  // on start-up
      .WillOnce(Invoke([&request_is_passed_to_worker_thread_promise] {
        request_is_passed_to_worker_thread_promise.set_value();
        return false;
      }))                             // for first request
      .WillRepeatedly(Return(true));  // for all following requests

  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);
  auto error_code = protocol.Send(NetworkRequest(), 0, nullptr, nullptr,
                                  nullptr, nullptr, nullptr);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));
  EXPECT_TRUE(underlying_protocol->m_sent_ids.empty());

  request_is_passed_to_worker_thread_future.wait();

  error_code = protocol.Send(NetworkRequest(), 1, nullptr, nullptr, nullptr,
                             nullptr, nullptr);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));
  underlying_protocol->wait_until_promises_are_satisfied();
  EXPECT_THAT(underlying_protocol->m_sent_ids.size(), Eq(2u));
}

namespace {
NetworkRequest get_sat_request() {
  NetworkRequest request;
  request.AddHeader("HEREServiceId", "satellitemaps_v2");
  return request;
}

bool satellite_selector(const NetworkRequest& request) {
  return any_of(begin(request.ExtraHeaders()), end(request.ExtraHeaders()),
                [](const std::pair<std::string, std::string>& header) {
                  return header.first == "HEREServiceId" &&
                         header.second == "satellitemaps_v2";
                });
}
}  // namespace

TEST(TestNetworkPriorityQueueDecorator, satellite_quota_test) {
  const auto number_of_requests = 3;
  auto underlying_protocol = std::make_shared<NiceMock<AccumulateSentIds> >();
  underlying_protocol->m_send_done =
      std::vector<std::promise<void> >(number_of_requests);
  underlying_protocol->m_callbacks =
      std::vector<Network::Callback>(number_of_requests);

  ON_CALL(*underlying_protocol, Ready()).WillByDefault(Return(true));

  NetworkRequestPriorityQueueDecorator protocol(
      underlying_protocol, std::numeric_limits<std::size_t>::max(),
      {std::make_pair(1, satellite_selector)});

  const auto callback = [](const NetworkResponse&) {};

  auto error_code = protocol.Send(get_sat_request(), 0, nullptr, nullptr,
                                  nullptr, nullptr, callback);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));
  error_code = protocol.Send(get_sat_request(), 1, nullptr, nullptr, nullptr,
                             nullptr, callback);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));

  error_code = protocol.Send(NetworkRequest(), 2, nullptr, nullptr, nullptr,
                             nullptr, callback);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));

  underlying_protocol->m_send_done[0].get_future().wait();
  underlying_protocol->m_send_done[2].get_future().wait();

  EXPECT_THAT(underlying_protocol->m_sent_ids.size(), Eq(2u));
  EXPECT_THAT(underlying_protocol->m_sent_ids, UnorderedElementsAre(2, 0));

  underlying_protocol->m_callbacks[0](
      olp::network::NetworkResponse(0, 200, ""));

  underlying_protocol->m_send_done[1].get_future().wait();
  EXPECT_THAT(underlying_protocol->m_sent_ids.size(), Eq(3u));
  EXPECT_THAT(underlying_protocol->m_sent_ids[2], Eq(1));

  underlying_protocol->m_callbacks.clear();
}

TEST(TestNetworkPriorityQueueDecorator, satellite_quota_cancel_test) {
  const auto number_of_requests = 3;
  const auto underlying_protocol =
      std::make_shared<NiceMock<AccumulateSentIds> >();
  underlying_protocol->m_send_done =
      std::vector<std::promise<void> >(number_of_requests);
  underlying_protocol->m_callbacks =
      std::vector<Network::Callback>(number_of_requests);

  EXPECT_CALL(*underlying_protocol, Ready())
      .WillOnce(Return(true))
      .WillRepeatedly(Return(true));

  NetworkRequestPriorityQueueDecorator protocol(
      underlying_protocol, std::numeric_limits<std::size_t>::max(),
      {std::make_pair(1, satellite_selector)});
  const auto callback = [](const NetworkResponse&) {};

  auto error_code = protocol.Send(get_sat_request(), 0, nullptr, nullptr,
                                  nullptr, nullptr, callback);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));
  error_code = protocol.Send(get_sat_request(), 1, nullptr, nullptr, nullptr,
                             nullptr, callback);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));

  error_code = protocol.Send(NetworkRequest(), 2, nullptr, nullptr, nullptr,
                             nullptr, callback);
  EXPECT_THAT(error_code, Eq(olp::network::NetworkProtocol::ErrorNone));

  underlying_protocol->m_send_done[0].get_future().wait();
  underlying_protocol->m_send_done[2].get_future().wait();

  protocol.Cancel(1);
  underlying_protocol->m_callbacks[0](
      olp::network::NetworkResponse(0, 200, ""));

  EXPECT_THAT(underlying_protocol->m_sent_ids.size(), Eq(2u));
  EXPECT_THAT(underlying_protocol->m_sent_ids, UnorderedElementsAre(0, 2));

  underlying_protocol->m_callbacks.clear();
}

// behaviour taken from underlying protocol
TEST(TestNetworkPriorityQueueDecorator,
     initialize_is_forwarded_to_the_underlying_protocol) {
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  EXPECT_CALL(*underlying_protocol, Initialize())
      .WillOnce(Return(true))
      .WillOnce(Return(false));

  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  EXPECT_TRUE(protocol.Initialize());
  EXPECT_FALSE(protocol.Initialize());
}

TEST(TestNetworkPriorityQueueDecorator,
     initialized_is_forwarded_to_the_underlying_protocol) {
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  EXPECT_CALL(*underlying_protocol, Initialized())
      .WillOnce(Return(true))
      .WillOnce(Return(false));

  const NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  EXPECT_TRUE(protocol.Initialized());
  EXPECT_FALSE(protocol.Initialized());
}

TEST(TestNetworkPriorityQueueDecorator,
     deinitialize_is_forwarded_to_the_underlying_protocol) {
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  EXPECT_CALL(*underlying_protocol, Deinitialize());

  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  protocol.Deinitialize();
}

TEST(TestNetworkPriorityQueueDecorator,
     amountPending_is_forwarded_to_the_underlying_protocol) {
  const auto pending_requests = 42u;
  const auto underlying_protocol =
      std::make_shared<NiceMock<NetworkProtocolMock> >();
  EXPECT_CALL(*underlying_protocol, AmountPending())
      .WillOnce(Return(pending_requests));

  NetworkRequestPriorityQueueDecorator protocol(underlying_protocol);

  EXPECT_THAT(protocol.AmountPending(), Eq(pending_requests));
}
