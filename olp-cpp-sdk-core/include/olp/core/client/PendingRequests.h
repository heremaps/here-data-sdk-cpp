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
 * @brief Container for not yet finished requests.
 */
class PendingRequests final {
 public:
  /**
   * @brief Cancels all pending tasks
   * @note This call does not wait for the tasks to finalize, use
   * CancelAllAndWait() to also wait for the tasks to finalize..
   * @return True on success
   */
  bool CancelAll();

  /**
   * @brief Cancels all pending tasks and waits for all beeing finalized.
   * @return True on success
   */
  bool CancelAllAndWait();

  /**
   * @brief Inserts task context into the requests container.
   * @param task_context Task context.
   */
  void Insert(TaskContext task_context);

  /**
   * @brief Removes a task context.
   * @param task_context Task context.
   */
  void Remove(TaskContext task_context);

 private:
  using ContextMap = std::unordered_set<TaskContext, TaskContextHash>;
  ContextMap task_contexts_;
  std::mutex task_contexts_lock_;
};

}  // namespace client
}  // namespace olp
