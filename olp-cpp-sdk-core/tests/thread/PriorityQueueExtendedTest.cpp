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

#include <queue>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "thread/PriorityQueueExtended.h"

namespace {
namespace thread = olp::thread;

struct TestObject {
  int value;
  int id;
};

bool operator<(const TestObject& lhs, const TestObject& rhs) {
  return lhs.value < rhs.value;
}

TEST(PriorityQueueExtendedTest, BasicFunctionality) {
  {
    SCOPED_TRACE("empty");

    thread::PriorityQueueExtended<int> queue;

    EXPECT_TRUE(queue.empty());
  }

  {
    SCOPED_TRACE("not empty");

    thread::PriorityQueueExtended<int> queue;
    queue.push(1);

    EXPECT_FALSE(queue.empty());
  }

  {
    SCOPED_TRACE("push lvalue");

    std::string value = "value";
    thread::PriorityQueueExtended<std::string> queue;
    queue.push(value);

    EXPECT_FALSE(value.empty());
    ASSERT_FALSE(queue.empty());
    EXPECT_FALSE(queue.front().empty());
    EXPECT_EQ(value, queue.front());
  }

  {
    SCOPED_TRACE("push rvalue");

    std::string value = "value";
    thread::PriorityQueueExtended<std::string> queue;
    queue.push(std::move(value));

    EXPECT_FALSE(value.empty());
    ASSERT_FALSE(queue.empty());
    EXPECT_FALSE(queue.front().empty());
  }

  {
    SCOPED_TRACE("front");

    auto value = 100;
    thread::PriorityQueueExtended<int> queue;
    queue.push(value);

    ASSERT_FALSE(queue.empty());
    EXPECT_EQ(queue.front(), value);
  }

  {
    SCOPED_TRACE("front const");

    auto value = 100;
    thread::PriorityQueueExtended<int> queue;
    queue.push(value);

    auto const& const_queue = queue;

    ASSERT_FALSE(const_queue.empty());
    EXPECT_EQ(const_queue.front(), value);
  }

  {
    SCOPED_TRACE("pop");

    thread::PriorityQueueExtended<int> queue;
    queue.push(100);

    ASSERT_FALSE(queue.empty());
    queue.pop();
    ASSERT_TRUE(queue.empty());
  }

  {
    SCOPED_TRACE("pop empty");

    thread::PriorityQueueExtended<int> queue;

    ASSERT_TRUE(queue.empty());
    queue.pop();
    ASSERT_TRUE(queue.empty());
  }
}

TEST(PriorityQueueExtendedTest, Priority) {
  thread::PriorityQueueExtended<int> queue;
  ASSERT_TRUE(queue.empty());

  // fill queue with data
  std::vector<int> priorities{3, 2, 1, 2};
  for (auto i : priorities) {
    queue.push(i);
    ASSERT_FALSE(queue.empty());
  }
  std::sort(priorities.begin(), priorities.end());

  // check queue is sorted
  for (auto it = priorities.rbegin(); it != priorities.rend(); ++it) {
    ASSERT_EQ(queue.front(), *it);
    ASSERT_FALSE(queue.empty());
    queue.pop();
  }

  ASSERT_TRUE(queue.empty());
}

TEST(PriorityQueueExtendedTest, FIFO) {
  thread::PriorityQueueExtended<TestObject> queue;
  ASSERT_TRUE(queue.empty());

  // fill queue with data
  std::vector<int> priorities{3, 2, 1, 2, 1, 3};
  int id = 0;
  for (auto i : priorities) {
    queue.push({i, id});
    ASSERT_FALSE(queue.empty());
    ++id;
  }

  // check objects with same priority stored in FIFO order
  auto top_obj = queue.front();
  queue.pop();
  while (!queue.empty()) {
    const auto& obj = queue.front();

    ASSERT_TRUE(obj < top_obj ||
                (obj.value == top_obj.value && obj.id > top_obj.id));
    ASSERT_FALSE(queue.empty());

    top_obj = obj;
    queue.pop();
  }

  ASSERT_TRUE(queue.empty());
}

}  // namespace
