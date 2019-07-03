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

#include "NetworkRequestPriorityQueue.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace olp::network;

using ::testing::Eq;
using ::testing::NotNull;

TEST(TestNetworkRequestPriorityQueue, testBasicPushPop) {
  const auto request_id = Network::NetworkRequestIdMin;
  NetworkRequestPriorityQueue queue;
  const RequestContextPtr context =
      std::make_shared<RequestContext>(NetworkRequest(), request_id, nullptr,
                                       nullptr, nullptr, nullptr, nullptr);
  queue.Push(context);
  GTEST_ASSERT_EQ(1u, queue.Size());
  const auto ret = queue.Pop();
  GTEST_ASSERT_EQ(0u, queue.Size());
}

TEST(TestNetworkRequestPriorityQueue, testPushPopWithDifferentPriorities) {
  const auto request_id = Network::NetworkRequestIdMin;
  const int priority1 = 1;
  const int priority2 = 2;
  const int priority3 = 3;
  NetworkRequestPriorityQueue queue;
  const auto context1 = std::make_shared<RequestContext>(
      NetworkRequest("", 0, priority2), request_id, nullptr, nullptr, nullptr,
      nullptr, nullptr);
  queue.Push(context1);
  const auto context2 = std::make_shared<RequestContext>(
      NetworkRequest("", 0, priority1), request_id, nullptr, nullptr, nullptr,
      nullptr, nullptr);
  queue.Push(context2);
  const auto context3 = std::make_shared<RequestContext>(
      NetworkRequest("", 0, priority3), request_id, nullptr, nullptr, nullptr,
      nullptr, nullptr);
  queue.Push(context3);

  GTEST_ASSERT_EQ(3u, queue.Size());
  const auto ret1 = queue.Pop();
  GTEST_ASSERT_EQ(ret1->request_.Priority(), priority3);
  const auto ret2 = queue.Pop();
  GTEST_ASSERT_EQ(ret2->request_.Priority(), priority2);
  const auto ret3 = queue.Pop();
  GTEST_ASSERT_EQ(ret3->request_.Priority(), priority1);

  GTEST_ASSERT_EQ(0u, queue.Size());
}

TEST(TestNetworkRequestPriorityQueue, testRemoveIf) {
  const auto request_id1 = Network::NetworkRequestIdMin;
  const auto request_id2 = request_id1 + 1;
  NetworkRequestPriorityQueue queue;
  auto context =
      std::make_shared<RequestContext>(NetworkRequest(), request_id1, nullptr,
                                       nullptr, nullptr, nullptr, nullptr);
  queue.Push(context);
  context =
      std::make_shared<RequestContext>(NetworkRequest(), request_id1, nullptr,
                                       nullptr, nullptr, nullptr, nullptr);
  queue.Push(context);
  context =
      std::make_shared<RequestContext>(NetworkRequest(), request_id2, nullptr,
                                       nullptr, nullptr, nullptr, nullptr);
  queue.Push(context);
  context =
      std::make_shared<RequestContext>(NetworkRequest(), request_id2, nullptr,
                                       nullptr, nullptr, nullptr, nullptr);
  queue.Push(context);
  context =
      std::make_shared<RequestContext>(NetworkRequest(), request_id2, nullptr,
                                       nullptr, nullptr, nullptr, nullptr);
  queue.Push(context);

  const auto removed_requests = queue.RemoveIf(
      [&](RequestContextPtr ptr) { return ptr->id_ == request_id1; });

  EXPECT_THAT(removed_requests.size(), Eq(2u));
  ASSERT_THAT(removed_requests[0], NotNull());
  EXPECT_THAT(removed_requests[0]->id_, request_id1);
  ASSERT_THAT(removed_requests[1], NotNull());
  EXPECT_THAT(removed_requests[1]->id_, request_id1);

  GTEST_ASSERT_EQ(3u, queue.Size());
}
