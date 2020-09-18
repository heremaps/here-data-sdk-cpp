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

#include <utility>

#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/utils/WarningWorkarounds.h>

namespace olp {
namespace thread {

/// Helper priority enum. NORMAL is a default value.
enum Priority : uint32_t { LOW = 100, NORMAL = 500, HIGH = 1000 };

/**
 * @brief An abstract interface that is used as a base for the custom thread
 * scheduling strategy.
 *
 * Subclasses should inherit from this class and implement virtual
 * `EnqueueTask` that takes any callable target (lambda expression, bind
 * expression, or any other function object) as input and adds it to
 * the execution pipeline.
 *
 */
class CORE_API TaskScheduler {
 public:
  /// Alias for the abstract interface input.
  using CallFuncType = std::function<void()>;

  virtual ~TaskScheduler() = default;

  /**
   * @brief Schedules the asynchronous task.
   *
   * @note Tasks added with this method has Priority::NORMAL priority.
   *
   * @param[in] func The callable target that should be added to the scheduling
   * pipeline.
   */
  void ScheduleTask(CallFuncType&& func) { EnqueueTask(std::move(func)); }

  /**
   * @brief Schedules the asynchronous task.
   *
   * @param[in] func The callable target that should be added to the scheduling
   * pipeline.
   * @param[in] priority The priority of the task. Tasks with higher priority
   * executes earlier.
   */
  void ScheduleTask(CallFuncType&& func, uint32_t priority) {
    EnqueueTask(std::move(func), priority);
  }

  /**
   * @brief Schedules the asynchronous cancellable task.
   *
   * @param[in] func The callable target that should be added to the scheduling
   * pipeline. As `CancellationContext` is created internally, it is passed as
   * input, so the callable function should have the following signature:
   *
   * @code
   *     void func(CancellationContext& context);
   * @endcode
   *
   * @return Returns the \c CancellationContext copy to the caller. The copy can
   * be used to cancel the enqueued tasks. Tasks can only be canceled before or
   * during execution if the task itself is designed to support this. Tasks are
   * also able to cancel the operation themselves as they get a non-const
   * reference to the \c CancellationContext class.
   */
  template <class Function, typename std::enable_if<!std::is_convertible<
                                decltype(std::declval<Function>()),
                                CallFuncType>::value>::type* = nullptr>
  client::CancellationContext ScheduleTask(Function&& func) {
    client::CancellationContext context;
    auto task = [func, context]() {
      if (!context.IsCancelled()) {
        func(context);
      };
    };
    EnqueueTask(std::move(task));
    return context;
  }

 protected:
  /**
   * @brief The abstract enqueue task interface that is implemented by
   * the subclass.
   *
   * Implement this method in the subclass that takes `TaskScheduler`
   * as a base and provides a custom algorithm for scheduling tasks
   * enqueued by the SDK.
   *
   * @note Tasks added trough this method should be scheduled with
   * Priority::NORMAL priority.
   *
   * @param[in] func The rvalue reference of the task that should be enqueued.
   * Move this task into your queue. No internal references are
   * kept. Once this method is called, you own the task.
   */
  virtual void EnqueueTask(CallFuncType&&) = 0;

  /**
   * @brief The enqueue task with priority interface that is implemented by the
   * subclass.
   *
   * Implement this method in the subclass that takes `TaskScheduler`
   * as a base and provides a custom algorithm for scheduling tasks
   * enqueued by the SDK. The method priorities tasks, so tasks with higher
   * priority executed earlier. Tasks within the same priority group should keep
   * the order.
   *
   * @param[in] func The rvalue reference of the task that should be enqueued.
   * Move this task into your queue. No internal references are
   * kept. Once this method is called, you own the task.
   * @param[in] priority The priority of the task. Tasks with higher priority
   * executes earlier.
   */
  virtual void EnqueueTask(CallFuncType&& func, uint32_t priority) {
    OLP_SDK_CORE_UNUSED(priority);
    EnqueueTask(std::forward<CallFuncType>(func));
  }
};

}  // namespace thread
}  // namespace olp
