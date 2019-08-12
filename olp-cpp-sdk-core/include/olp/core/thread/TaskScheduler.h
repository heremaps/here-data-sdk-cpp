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

#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>

namespace olp {
namespace thread {

/**
 * @brief The TaskScheduler class is an abstract interface to be used as base
 * for a custom thread scheduling strategy.
 *
 * Subclasses should inherit from this class and implement virtual EnqueueTask,
 * that takes any callable target (lambda expression, bind expression or any
 * other function object) as input and adds it to the execution pipeline.
 *
 */
class CORE_API TaskScheduler {
 public:
  /// Alias for abstract interface input.
  using CallFuncType = std::function<void()>;

  virtual ~TaskScheduler() = default;

  /**
   * @brief Use this method to schedule a asynchronous task.
   * @param[in] func The callable target to be added to the scheduling pipeline.
   */
  void ScheduleTask(CallFuncType&& func) { EnqueueTask(std::move(func)); }

  /**
   * @brief Use this methods to schedule a asynchronous cancellable task.
   * @param[in] func The callable target to be added to the scheduling pipeline.
   * As the CancellationContext created internally will be passed as input, the
   * func callable should have the following signature:
   * @code
   *     void func(CancellationContext& context);
   * @encode
   * @return Returns a \c CancellationContext copy to the caller which can be
   * used to cancel the enqueued tasks. Tasks can only be cancelled before
   * execution or during execution if the task itself is designed to support
   * this. Tasks are also able to cancel the operation themselves as they get a
   * non-const reference to the \c CancellationContext.
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
   * @brief Abstract enqueue task interface to be implemented by
   * subclass.
   *
   * Implement this method in your subclass taking TaskScheduler
   * as base and provide a custom algorithm for scheduling tasks
   * enqueued by the SDK.
   *
   * @param[in] func Rvalue reference of the task to be enqueued.
   * Move this task into your queue. No internal reference is
   * kept, once called you own the task.
   */
  virtual void EnqueueTask(CallFuncType&&) = 0;
};

}  // namespace thread
}  // namespace olp
