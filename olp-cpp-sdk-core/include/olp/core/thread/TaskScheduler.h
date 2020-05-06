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

namespace olp {
namespace thread {

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
   * @param[in] func The callable target that should be added to the scheduling
   * pipeline.
   */
  void ScheduleTask(CallFuncType&& func) { EnqueueTask(std::move(func)); }

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
   * @param[in] func The rvalue reference of the task that should be enqueued.
   * Move this task into your queue. No internal references are
   * kept. Once this method is called, you own the task.
   */
  virtual void EnqueueTask(CallFuncType&&) = 0;
};

}  // namespace thread
}  // namespace olp
