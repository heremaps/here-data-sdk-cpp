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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

#include <olp/core/thread/SyncQueue.h>

using SyncTaskType = std::function<void()>;
using SyncQueueFifo = olp::thread::SyncQueueFifo<SyncTaskType>;

using SyncIncType = std::function<void(size_t)>;
using SyncQueueIncFifo = olp::thread::SyncQueueFifo<SyncIncType>;

using SharedQueueType = std::shared_ptr<std::string>;
using SyncQueueShared = olp::thread::SyncQueueFifo<SharedQueueType>;

using namespace ::testing;
using namespace std::chrono;

TEST(SyncQueueTest, initialize) {
  SCOPED_TRACE("Default initialized not closed");
  SyncQueueFifo sync_queue;
  bool check = false;
  sync_queue.Push([&check]() { check = true; });

  SyncTaskType element;
  EXPECT_TRUE(sync_queue.Pull(element));
  ASSERT_TRUE(element) << "Push() or Pull() failed.";

  element();

  EXPECT_TRUE(sync_queue.Empty());
  EXPECT_TRUE(check);
}

TEST(SyncQueueTest, push) {
  {
    SCOPED_TRACE("Push supports rvalues");
    SyncQueueShared sync_queue;

    auto string = std::make_shared<std::string>("rvalue");
    sync_queue.Push(std::move(string));

    EXPECT_FALSE(sync_queue.Empty());
    EXPECT_FALSE(string) << "string should be moved";
  }
  {
    SCOPED_TRACE("Push supports lvalues");
    SyncQueueShared sync_queue;

    auto string = std::make_shared<std::string>("lvalue");
    sync_queue.Push(string);

    EXPECT_FALSE(sync_queue.Empty());
    EXPECT_TRUE(string && !string->empty());
  }
  {
    SCOPED_TRACE("Push on closed queue");
    SyncQueueShared sync_queue;
    sync_queue.Close();

    auto string = std::make_shared<std::string>("value");
    sync_queue.Push(string);
    sync_queue.Push(std::move(string));

    string.reset();
    EXPECT_TRUE(sync_queue.Empty());
    EXPECT_FALSE(sync_queue.Pull(string));
    EXPECT_FALSE(string);
  }
}

TEST(SyncQueueTest, pull) {
  {
    SCOPED_TRACE("Pull on open queue");
    SyncQueueFifo sync_queue;

    bool check = false;
    sync_queue.Push([&check]() { check = true; });
    EXPECT_FALSE(sync_queue.Empty());
    EXPECT_FALSE(check);

    // Pull and execute
    SyncTaskType task;
    EXPECT_TRUE(sync_queue.Pull(task));
    ASSERT_TRUE(task);

    task();
    EXPECT_TRUE(check);
  }
  {
    SCOPED_TRACE("Pull on closed queue");
    SyncQueueFifo sync_queue;

    bool check = false;
    sync_queue.Push([&check]() { check = true; });
    EXPECT_FALSE(sync_queue.Empty());
    EXPECT_FALSE(check);

    // Close queue
    sync_queue.Close();

    SyncTaskType task;
    EXPECT_FALSE(sync_queue.Pull(task));
    EXPECT_FALSE(task);
  }
}

TEST(SyncQueueTest, close) {
  SCOPED_TRACE("Close should delete all elements");
  SyncQueueShared sync_queue;

  auto string1 = std::make_shared<std::string>("close1");
  std::weak_ptr<std::string> weak1 = string1;
  auto string2 = std::make_shared<std::string>("close2");
  std::weak_ptr<std::string> weak2 = string2;

  sync_queue.Push(std::move(string1));
  sync_queue.Push(std::move(string2));
  EXPECT_FALSE(sync_queue.Empty());

  ASSERT_TRUE(weak1.lock());
  ASSERT_TRUE(weak2.lock());

  // Close the queue, weak_ptrs should be empty
  sync_queue.Close();
  EXPECT_TRUE(sync_queue.Empty());
  ASSERT_FALSE(weak1.lock());
  ASSERT_FALSE(weak2.lock());
}

TEST(SyncQueueTest, concurrent_usage) {
  SCOPED_TRACE("Concurrent Push/Pull until queue is closed");

  // One thread pushes tasks, the others handle the tasks
  // Use counter to make sure each tasks run at least once.
  constexpr uint32_t kNumThreads = 5;
  constexpr milliseconds kSleep(300);
  std::vector<uint32_t> thread_counter(kNumThreads, 0u);
  std::vector<std::thread> threads;
  SyncQueueIncFifo sync_queue;
  std::atomic<uint32_t> counter(0u);

  // Start threads push tasks to be handled
  for (size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back([this, idx, &sync_queue] {
      for (;;) {
        SyncIncType task;
        if (!sync_queue.Pull(task)) {
          return;
        }

        // Execute task
        task(idx);
      }
    });
  }

  for (uint32_t idx = 0u; idx < kNumThreads; ++idx) {
    sync_queue.Push([&](size_t idx) {
      // Increment counter and idx in vector to mark that each thread executed
      // one task and did not get blocked by any other thread.
      if (idx < thread_counter.size()) {
        thread_counter[idx]++;
      }
      ++counter;

      std::this_thread::sleep_for(kSleep);
    });
  }

  // Wait for threads to finish but do not exceed 1min
  const auto start = system_clock::now();
  auto check_condition = [&]() {
    return counter.load() < kNumThreads &&
           duration_cast<milliseconds>(system_clock::now() - start).count() <
               1000u;
  };

  while (!sync_queue.Empty() && check_condition()) {
    std::this_thread::sleep_for(kSleep / 3);
  }

  // Each thread should have run once, each task executed
  const std::vector<uint32_t> expected(kNumThreads, 1u);
  EXPECT_THAT(thread_counter, ContainerEq(expected));
  EXPECT_EQ(kNumThreads, counter.load());

  // Close and join all threads
  sync_queue.Close();
  for (auto& t : threads) {
    t.join();
  }
}
