/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <olp/core/thread/TaskScheduler.h>

namespace olp {
namespace thread {

/**
 * @brief An implementation of the `TaskScheduler` instance that uses a thread
 * pool.
 *
 */
class CORE_API ThreadPoolTaskScheduler final : public TaskScheduler {
 public:
  /**
   * @brief Creates the `ThreadPoolTaskScheduler` object with one thread.
   *
   * @param thread_count The number of threads initialized in the thread pool.
   */
  explicit ThreadPoolTaskScheduler(size_t thread_count = 1u);

  /**
   * @brief Closes the `SyncQueue` instance and joins threads.
   */
  ~ThreadPoolTaskScheduler() override;

  /// Non-copyable, non-movable
  ThreadPoolTaskScheduler(const ThreadPoolTaskScheduler&) = delete;
  /// Non-copyable, non-movable
  ThreadPoolTaskScheduler& operator=(const ThreadPoolTaskScheduler&) = delete;
  /// Non-copyable, non-movable
  ThreadPoolTaskScheduler(ThreadPoolTaskScheduler&&) = delete;
  /// Non-copyable, non-movable
  ThreadPoolTaskScheduler& operator=(ThreadPoolTaskScheduler&&) = delete;

 protected:
  /**
   * @brief Overrides the base class method to enqueue tasks and execute them on
   * the next free thread from the thread pool.
   *
   * @note Tasks added with this method has Priority::NORMAL priority.
   *
   * @param func The rvalue reference of the task that should be enqueued.
   * Move this task into your queue. No internal references are
   * kept. Once this method is called, you own the task.
   */
  void EnqueueTask(TaskScheduler::CallFuncType&& func) override;

  /**
   * @brief Overrides the base class method to enqueue tasks and execute them on
   * the next free thread from the thread pool.
   *
   * @param func The rvalue reference of the task that should be enqueued.
   * Move this task into your queue. No internal references are
   * kept. Once this method is called, you own the task.
   * @param priority The priority of the task. Tasks with higher priority
   * executes earlier.
   */
  void EnqueueTask(TaskScheduler::CallFuncType&& func,
                   uint32_t priority) override;

 private:
  class QueueImpl;

  /// Thread pool created in constructor.
  std::vector<std::thread> thread_pool_;
  /// SyncQueue used to manage tasks.
  std::unique_ptr<QueueImpl> queue_;
};

}  // namespace thread
}  // namespace olp
