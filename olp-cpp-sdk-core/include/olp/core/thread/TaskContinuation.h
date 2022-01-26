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

#pragma once

#include <deque>
#include <functional>
#include <memory>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/TaskContext.h>
#include <olp/core/thread/Continuation.h>
#include <olp/core/thread/ExecutionContext.h>
#include <olp/core/thread/TypeHelpers.h>

namespace olp {
namespace thread {

class TaskScheduler;

/// Creates a chain of tasks for an asynchronous execution.
class CORE_API TaskContinuation final {
 public:
  /**
   * @brief Creates the `TaskContinuation` instance.
   *
   * @param scheduler The `TaskScheduler` instance.
   */
  explicit TaskContinuation(std::shared_ptr<thread::TaskScheduler> scheduler);

  /**
   * @brief Initializes the `Continuation` object.
   *
   * It creates the `Continuation` instance with a template callback
   * as a parameter, which is the first task in the task continuation chain.
   *
   * @param task A task you want to add to the continuation chain.
   */
  template <typename Callable>
  Continuation<internal::AsyncResultType<Callable>> Then(Callable task);

 private:
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  ExecutionContext execution_context_;
};

}  // namespace thread
}  // namespace olp

#include "TaskContinuation.inl"
