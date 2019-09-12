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

#include <olp/core/client/CancellationContext.h>
#include <olp/core/thread/ThreadPoolTaskScheduler.h>

using SyncTaskType = std::function<void()>;
using CancellationContext = olp::client::CancellationContext;
using TaskScheduler = olp::thread::TaskScheduler;
using ThreadPool = olp::thread::ThreadPoolTaskScheduler;

using namespace ::testing;
using namespace std::chrono;

namespace {
constexpr size_t kThreads{3u};
constexpr size_t kNumTasks{30u};
constexpr milliseconds kSleep{100};
constexpr int64_t kMaxWaitMs{1000};
}  // namespace

TEST(ThreadPoolTaskSchedulerTest, single_user_push) {
  SCOPED_TRACE("Single user pushes tasks");

  // Start thread pool
  auto thread_pool = std::make_shared<ThreadPool>(kThreads);
  TaskScheduler& scheduler = *thread_pool;
  std::atomic<uint32_t> counter(0u);

  // Allow threads to start
  std::this_thread::sleep_for(kSleep);

  // Add tasks to the queue, threads should start executing them
  for (uint32_t idx = 0u; idx < kNumTasks; ++idx) {
    scheduler.ScheduleTask([&](const CancellationContext&) { ++counter; });
    scheduler.ScheduleTask([&]() { ++counter; });
  }

  // Wait for threads to finish but do not exceed 1min
  const auto start = system_clock::now();
  constexpr size_t expected_tasks = 2 * kNumTasks;

  auto check_condition = [&]() {
    return counter.load() < expected_tasks &&
           duration_cast<milliseconds>(system_clock::now() - start).count() <
               kMaxWaitMs;
  };

  while (check_condition()) {
    std::this_thread::sleep_for(kSleep);
  }

  EXPECT_EQ(expected_tasks, counter.load());

  // Close queue and join threads.
  // SyncQueue and threads join should be done in destructor.
  thread_pool.reset();
}

TEST(ThreadPoolTaskSchedulerTest, multi_user_push) {
  SCOPED_TRACE("Multiple users push tasks");

  constexpr uint32_t kPushThreads = 3;
  constexpr uint32_t kTotalTasks = kPushThreads * (2 * kNumTasks);

  // Start thread pool
  auto thread_pool = std::make_shared<ThreadPool>(kThreads);
  std::atomic<uint32_t> counter(0u);
  std::vector<std::thread> push_threads;

  // Allow threads to start
  std::this_thread::sleep_for(kSleep);

  // Create and start push threads for concurrent task creation
  push_threads.reserve(kPushThreads);
  for (size_t idx = 0; idx < kPushThreads; ++idx) {
    push_threads.emplace_back([=, &counter] {
      TaskScheduler& scheduler = *thread_pool;
      for (uint32_t idx = 0u; idx < kNumTasks; ++idx) {
        scheduler.ScheduleTask([&](const CancellationContext&) { ++counter; });
        scheduler.ScheduleTask([&]() { ++counter; });
        std::this_thread::sleep_for(kSleep / 100);
      }
    });
  }

  // Wait for threads to finish but do not exceed 1min
  const auto start = system_clock::now();
  auto check_condition = [&]() {
    return counter.load() < kTotalTasks &&
           duration_cast<milliseconds>(system_clock::now() - start).count() <
               kMaxWaitMs;
  };

  while (check_condition()) {
    std::this_thread::sleep_for(kSleep);
  }

  EXPECT_EQ(kTotalTasks, counter.load());

  // Close queue and join threads.
  // SyncQueue and threads join should be done in destructor.
  thread_pool.reset();

  for (auto& thread : push_threads) {
    thread.join();
  }
  push_threads.clear();
}
