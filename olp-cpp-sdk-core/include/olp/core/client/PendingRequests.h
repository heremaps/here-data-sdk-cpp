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

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/TaskContext.h>

namespace olp {
namespace client {

/**
 * @brief A container for requests that have not finished yet.
 */
class CORE_API PendingRequests final {
 public:
  /**
   * @brief Cancels all the pending tasks.
   *
   * @note This call does not wait for the tasks to finalize. To wait for
   * the tasks to finalize, use `CancelAllAndWait()`.
   *
   * @return True on success; false otherwise.
   */
  bool CancelAll();

  /**
   * @brief Cancels all the pending tasks and waits for the tasks that are
   * finalizing.
   *
   * @return True on success; false otherwise.
   */
  bool CancelAllAndWait();

  /**
   * @brief Inserts the task context into the request container.
   *
   * @param task_context The `TaskContext` instance.
   */
  void Insert(TaskContext task_context);

  /**
   * @brief Removes the task context.
   *
   * @param task_context The `TaskContext` instance.
   */
  void Remove(TaskContext task_context);

  /**
   * @brief Gets the number of tasks.
   *
   * @return The number of tasks pending.
   */
  size_t GetTaskCount() const;

 private:
  using ContextMap = std::unordered_set<TaskContext, TaskContextHash>;
  ContextMap task_contexts_;
  mutable std::mutex task_contexts_lock_;
};

}  // namespace client
}  // namespace olp
