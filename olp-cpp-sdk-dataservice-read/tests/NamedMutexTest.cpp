/*
 * Copyright (C) 2022 HERE Europe B.V.
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
#include <thread>

#include "repositories/NamedMutex.h"

namespace {

namespace client = olp::client;
namespace repository = olp::dataservice::read::repository;

TEST(NamedMutexTest, Lock) {
  repository::NamedMutexStorage storage;

  client::CancellationContext context;

  repository::NamedMutex main_mutex(storage, "mutex", context);
  main_mutex.lock();

  std::atomic_int counter{0};

  std::thread thread_1([&]() {
    repository::NamedMutex mutex(storage, "mutex", context);
    mutex.lock();
    counter.fetch_add(1);
    mutex.unlock();
  });

  std::thread thread_2([&]() {
    repository::NamedMutex mutex(storage, "mutex", context);
    mutex.lock();
    counter.fetch_add(1);
    mutex.unlock();
  });

  EXPECT_EQ(counter.load(), 0);

  main_mutex.unlock();

  thread_1.join();
  thread_2.join();

  EXPECT_EQ(counter.load(), 2);
}

TEST(NamedMutexTest, Cancel) {
  repository::NamedMutexStorage storage;

  client::CancellationContext main_context;
  client::CancellationContext thread_1_context;
  client::CancellationContext thread_2_context;

  repository::NamedMutex main_mutex(storage, "mutex", main_context);
  main_mutex.lock();

  std::atomic_int counter{0};

  std::thread thread_1([&]() {
    repository::NamedMutex mutex(storage, "mutex", thread_1_context);
    mutex.lock();
    if (thread_1_context.IsCancelled()) {
      counter.fetch_add(1);
    }
    mutex.unlock();
  });

  std::thread thread_2([&]() {
    repository::NamedMutex mutex(storage, "mutex", thread_2_context);
    mutex.lock();
    if (thread_2_context.IsCancelled()) {
      counter.fetch_add(1);
    }
    mutex.unlock();
  });

  EXPECT_EQ(counter.load(), 0);

  thread_1_context.CancelOperation();
  thread_2_context.CancelOperation();

  // Give other threads time to react.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(counter.load(), 2);

  main_mutex.unlock();

  thread_1.join();
  thread_2.join();
}

TEST(NamedMutexTest, CancelationLifetime) {
  repository::NamedMutexStorage storage;

  client::CancellationContext main_context;
  {
    repository::NamedMutex main_mutex(storage, "mutex", main_context);
    main_mutex.lock();
    main_mutex.unlock();
  }
  main_context.CancelOperation();
}

}  // namespace
