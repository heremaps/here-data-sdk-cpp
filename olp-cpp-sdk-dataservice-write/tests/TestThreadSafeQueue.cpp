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

#include <chrono>
#include <string>

#include <gtest/gtest.h>
#include "ThreadSafeQueue.h"

namespace {

using namespace olp::dataservice::write;

TEST(ThreadSafeQueueTest, EmptyQueue) {
  ThreadSafeQueue<int> queue;
  ASSERT_TRUE(queue.empty());
  ASSERT_EQ(0ull, queue.size());

  int i;
  ASSERT_FALSE(queue.try_pop(i));

  queue.push(1);
  ASSERT_FALSE(queue.empty());
  ASSERT_EQ(1ull, queue.size());

  queue.pop();
  ASSERT_TRUE(queue.empty());
  ASSERT_EQ(0ull, queue.size());
  ASSERT_FALSE(queue.try_pop(i));
}

TEST(ThreadSafeQueueTest, FrontBackQueue) {
  ThreadSafeQueue<int> queue;
  ASSERT_EQ(0, queue.top());

  // push a number of items onto the queue and check front/back/size/empty
  // functions.
  int j = 1;
  queue.push(j);
  ASSERT_FALSE(queue.empty());
  ASSERT_EQ(1ull, queue.size());
  ASSERT_EQ(1, queue.front());
  ASSERT_EQ(1, queue.back());
  ASSERT_EQ(1, queue.top());

  j = 3;
  queue.emplace(2);
  queue.push(std::move(j));
  ASSERT_EQ(3ull, queue.size());
  ASSERT_EQ(1, queue.front());
  ASSERT_EQ(3, queue.back());

  // pop items from the queue and check front/back/size/empty functions.
  queue.pop();
  ASSERT_EQ(2ull, queue.size());
  ASSERT_FALSE(queue.empty());
  ASSERT_EQ(2, queue.front());
  ASSERT_EQ(3, queue.back());

  int i;
  ASSERT_TRUE(queue.try_pop(i));
  ASSERT_EQ(2, i);
  ASSERT_FALSE(queue.empty());
  ASSERT_EQ(1ull, queue.size());

  ASSERT_TRUE(queue.try_pop(i));
  ASSERT_EQ(3, i);
  ASSERT_TRUE(queue.empty());
  ASSERT_EQ(0ull, queue.size());

  ASSERT_FALSE(queue.try_pop(i));
}

TEST(ThreadSafeQueueTest, EmplaceQueue) {
  ThreadSafeQueue<int> queue;
  queue.emplace(1);
  queue.emplace(2);
  queue.emplace(3);
  queue.emplace(4);
  queue.emplace(5);

  const auto& queue_ref = queue;
  ASSERT_EQ(5ull, queue_ref.size());
  ASSERT_EQ(1, queue_ref.front());
  ASSERT_EQ(5, queue_ref.back());

  ThreadSafeQueue<int> queue2;
  queue.swap(queue2);
  ASSERT_EQ(0ull, queue_ref.size());
  ASSERT_EQ(5ull, queue2.size());
}

TEST(ThreadSafeQueueTest, PushQueue) {
  ThreadSafeQueue<int> queue;
  queue.emplace(1);
  queue.emplace(2);
  queue.emplace(3);
  queue.emplace(4);
  queue.emplace(5);

  // push a 6, with queue size limit 10 and overwrite set to false.
  queue.push(6, 10, false);
  ASSERT_EQ(6ull, queue.size());
  ASSERT_EQ(1, queue.front());
  ASSERT_EQ(6, queue.back());

  int val = 7;
  queue.push(val, 10, true);
  ASSERT_EQ(7ull, queue.size());
  ASSERT_EQ(1, queue.front());
  ASSERT_EQ(7, queue.back());

  // try to push an 8 with queue size limited to 5, should not change queue as
  // size is already 7.
  queue.push(8, 5, false);
  ASSERT_EQ(7ull, queue.size());
  ASSERT_EQ(1, queue.front());
  ASSERT_EQ(7, queue.back());

  // allow overwrites, should modify queue.
  queue.push(8, 5, true);
  ASSERT_EQ(5ull, queue.size());
  ASSERT_EQ(4, queue.front());
  ASSERT_EQ(8, queue.back());

  // no change, not enough space
  queue.push(9, 5, false);
  ASSERT_EQ(5ull, queue.size());
  ASSERT_EQ(4, queue.front());
  ASSERT_EQ(8, queue.back());

  // enough space this time.
  queue.push(9, 6, false);
  ASSERT_EQ(6ull, queue.size());
  ASSERT_EQ(4, queue.front());
  ASSERT_EQ(9, queue.back());

  // overwrite with smaller size.
  int j = 10;
  queue.push(std::move(j), 1, true);
  ASSERT_EQ(1ull, queue.size());
  ASSERT_EQ(10, queue.front());
  ASSERT_EQ(10, queue.back());

  // push with max size 0, should leave unchanged due to overwrite false
  j = 11;
  queue.push(std::move(j), 0, false);
  ASSERT_EQ(1ull, queue.size());
  ASSERT_FALSE(queue.empty());

  // push with max size 0, should clear
  queue.push(std::move(j), 0, true);
  ASSERT_EQ(0ull, queue.size());
  ASSERT_TRUE(queue.empty());
  ASSERT_EQ(0, queue.top());
}

TEST(ThreadSafeQueueTest, MultiEmplacePopQueue) {
  ThreadSafeQueue<int> queue;
  queue.emplace(1);
  queue.emplace(2);
  queue.emplace(3);
  queue.emplace(4);
  queue.emplace(5);

  ASSERT_TRUE(queue.pop(3ull));
  ASSERT_EQ(2ull, queue.size());
  ASSERT_EQ(4, queue.front());
  ASSERT_EQ(5, queue.back());

  ASSERT_EQ(4, queue.top());
  ASSERT_EQ(5, queue.top(1ull));
  ASSERT_EQ(0, queue.top(2ull));

  queue.emplace_count(3ull, 7);
  ASSERT_EQ(5ull, queue.size());
  ASSERT_EQ(4, queue.front());
  ASSERT_EQ(7, queue.back());

  queue.emplace_count(0ll, 8);
  ASSERT_EQ(5ull, queue.size());
  ASSERT_EQ(4, queue.front());
  ASSERT_EQ(7, queue.back());

  ASSERT_FALSE(queue.pop(6ull));
}

TEST(ThreadSafeQueueTest, QueueString) {
  ThreadSafeQueue<std::string> queue;

  queue.push("test1");
  ASSERT_EQ(1ull, queue.size());
  ASSERT_EQ("test1", queue.front());
  ASSERT_EQ("test1", queue.back());
  ASSERT_EQ("test1", queue.top());

  std::string test2 = "test2";
  queue.push(test2);
  ASSERT_EQ(2ull, queue.size());
  ASSERT_EQ("test1", queue.front());
  ASSERT_EQ("test2", queue.back());
  ASSERT_EQ("test1", queue.top());

  std::string test3 = "test3";
  queue.push(std::move(test3));
  ASSERT_EQ(3ull, queue.size());
  ASSERT_EQ("test1", queue.front());
  ASSERT_EQ("test3", queue.back());
  ASSERT_EQ("test1", queue.top());
  ASSERT_EQ("", test3);

  std::string test4 = "test4";
  queue.push(std::move(test4), 3ull, false);
  ASSERT_EQ(3ull, queue.size());
  ASSERT_EQ("test1", queue.front());
  ASSERT_EQ("test3", queue.back());
  ASSERT_EQ("test1", queue.top());

  std::string test5 = "test5";
  queue.push(std::move(test5), 3ull, true);
  ASSERT_EQ(3ull, queue.size());
  ASSERT_EQ("test2", queue.front());
  ASSERT_EQ("test5", queue.back());
  ASSERT_EQ("test2", queue.top());
  EXPECT_EQ("", test5);

  std::string test6 = "test6";
  queue.push(std::move(test6), 4ull, false);
  ASSERT_EQ(4ull, queue.size());
  ASSERT_EQ("test2", queue.front());
  ASSERT_EQ("test6", queue.back());
  ASSERT_EQ("test2", queue.top());
  EXPECT_EQ("", test6);
}

}  // namespace
