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

#pragma once

#include <thread>
#include <vector>

#include <olp/core/thread/SyncQueue.h>
#include <olp/core/thread/TaskScheduler.h>

namespace olp {
namespace thread {

class CORE_API ThreadPoolTaskScheduler final : public TaskScheduler {
 public:
  /**
   * @brief Constructor with default thread_count set to 1.
   * @param[in] thread_count Number of threads to be initialized in the thread
   * pool.
   */
  ThreadPoolTaskScheduler(size_t thread_count = 1u);

  /**
   * @brief Destructor that will close the SyncQueue and join threads.
   */
  ~ThreadPoolTaskScheduler() override;

  // Non-copyable, non-movable
  ThreadPoolTaskScheduler(const ThreadPoolTaskScheduler&) = delete;
  ThreadPoolTaskScheduler& operator=(const ThreadPoolTaskScheduler&) = delete;
  ThreadPoolTaskScheduler(ThreadPoolTaskScheduler&&) = delete;
  ThreadPoolTaskScheduler& operator=(ThreadPoolTaskScheduler&&) = delete;

 protected:
  /// Override base class method to enqueue tasks and execute them eventually on
  /// the next free thread from the thread pool.
  void EnqueueTask(TaskScheduler::CallFuncType&& func) override;

 private:
  /// Thread pool created in constructor.
  std::vector<std::thread> thread_pool_;
  /// SyncQueue used to manage tasks.
  using SyncQueueFifoPtr =
      std::shared_ptr<SyncQueueFifo<TaskScheduler::CallFuncType>>;
  SyncQueueFifoPtr sync_queue_;
};

}  // namespace thread
}  // namespace olp
