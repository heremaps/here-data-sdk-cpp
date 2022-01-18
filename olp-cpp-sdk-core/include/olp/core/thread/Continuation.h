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
#include <olp/core/thread/ExecutionContext.h>
#include <olp/core/thread/TypeHelpers.h>

namespace olp {
namespace thread {

class TaskScheduler;

namespace internal {

/// An internal implementation of `Continuation`.
/// It provides mechanisms to create a chain of tasks, start, cancel,
/// and finalize an execution.
///
/// @note This is a private implementation class for internal use only, and not
/// bound to any API stability promises. Please do not use directly.
class CORE_API ContinuationImpl final {
 public:
  /// An alias of a function which returns an error as a callback.
  using FailedCallback = std::function<void(client::ApiError)>;
  /// Type of return value of Continuation Task.
  using OutResultType = std::unique_ptr<UntypedSmartPointer>;

  /// Type of Continuation Type.
  using TaskType = std::function<OutResultType(void*)>;
  /// Default callback type, uses void* as a parameter which is later converted
  /// to the specific type, callback doesn't have a return type.
  using CallbackType = std::function<void(void*)>;
  /// An internal type of tasks in Continuation.
  using AsyncTaskType = std::function<void(void*, CallbackType)>;
  /// An alias for a processing tasks finalization type.
  using FinalCallbackType = std::function<void(void*, bool)>;
  /// An alias of a pair of Task Continuation types.
  using ContinuationTask = std::pair<AsyncTaskType, TaskType>;

  /// A default constructor of `ContinuationImpl`.
  ContinuationImpl() = default;

  /**
   * @brief A contructor for a continuation using the given parameters.
   *
   * @param task_scheduler The `TaskScheduler` instance.
   * @param context The `ExecutionContext` instance.
   * @param task The `ContinuationTask` instance.
   */
  ContinuationImpl(std::shared_ptr<TaskScheduler> task_scheduler,
                   ExecutionContext context, ContinuationTask task);

  /**
   * @brief Adds an asynchronous task (one taking a callback) as the next
   * task of the `ContinuationImpl`.
   *
   * @param task A task for adding to the continuation chain.
   *
   * @return The `ContinuationImpl` instance.
   */
  ContinuationImpl Then(ContinuationTask task);

  /**
   * @brief Starts the execution of a task continuation chain.
   *
   * @param callback Handles the finalization of a task chain.
   */
  void Run(FinalCallbackType callback);

  /**
   * @brief Provides the execution context object.
   *
   * @return The `ExecutionContext` instance.
   */
  const ExecutionContext& GetExecutionContext() const;

  /**
   * @brief Provides cancellation context object.
   *
   * @return The `CancellationContext` instance.
   */
  const client::CancellationContext& GetContext() const;

  /**
   * @brief Provides a status whether CancellationContext is cancelled.
   *
   * @return True if Continuation is cancelled; false otherwise.
   */
  bool Cancelled() const;

  /**
   * @brief Sets a callback on calling SetError.
   *
   * @param callback Handles an execution finalization of a continuation chain.
   */
  void SetFailedCallback(FailedCallback callback);

  /**
   * @brief Clears the continuation chain tasks.
   */
  void Clear();

 private:
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  std::deque<ContinuationTask> tasks_;
  ExecutionContext execution_context_;

  /// a variable shows whether it is allowed to do the changes to the task
  /// chain, used to prevent using methods of `Continuation` after it was run.
  bool change_allowed{true};
};

}  // namespace internal

/// A Template for Continuation of generic type.
template <typename ResultType>
class CORE_API Continuation final {
  /// Response type alias of template class type.
  using Response = client::ApiResponse<ResultType, client::ApiError>;
  /// An alias for finalization callback.
  using FinallyCallbackType = std::function<void(Response)>;

  /// An alias for `ContinuationImplType::ContinuationImpl`.
  using ContinuationImplType = internal::ContinuationImpl;
  /// An alias for `ContinuationImplType::OutResultType`.
  using OutResultType = ContinuationImplType::OutResultType;
  /// An alias for `ContinuationImplType::TaskType`.
  using TaskType = ContinuationImplType::TaskType;
  /// An alias for `ContinuationImplType::CallbackType`.
  using CallbackType = ContinuationImplType::CallbackType;
  /// An alias for a Continuation chain first task.
  using ContinuationTaskType =
      std::function<void(ExecutionContext, std::function<void(ResultType)>)>;

 public:
  /**
   * @brief A contructor for a continuation using the given parameters.
   *
   * @param task_scheduler The `TaskScheduler` instance.
   * @param context The `ExecutionContext` instance.
   * @param task Continuation task's callback, constains thr execution context
   * and returns a value for the next task.
   */
  Continuation(std::shared_ptr<thread::TaskScheduler> scheduler,
               ExecutionContext context, ContinuationTaskType task);

  /**
   * @brief A contructor for a continuation using the given parameters.
   *
   * @param continuation The `ContinuationImpl` instance.
   */
  Continuation(ContinuationImplType continuation);

  /**
   * @brief Adds an asynchronous task (one taking a callback) as the next
   * step of the `Continuation`.
   *
   * @param task A task for adding to the continuation chain.
   */
  template <typename Callable>
  Continuation<internal::DeducedType<Callable>> Then(Callable task);

  /**
   * @brief Starts an execution of a task continuation chain.
   */
  void Run();

  /**
   * @brief Provides a token to cancel the task.
   *
   * @return The `CancellationToken` instance.
   */
  client::CancellationToken CancelToken();

  /**
   * @brief Adds an asynchronous task (one taking a callback) as the next
   * step of the  Continuation.
   *
   * @param execution_func A task for adding to the continuation chain.
   */
  template <typename NewType>
  Continuation<internal::RemoveRefAndConst<NewType>> Then(
      std::function<void(ExecutionContext, ResultType,
                         std::function<void(NewType)>)>
          execution_func);

  /**
   * @brief Handles the finalization of Continuation, sets a callback
   * to the existing Continuation instance.
   *
   * @param finally_callback Callback that handles both
   * successful and not successful results.
   *
   * @return The `Continuation` instance.
   */
  Continuation& Finally(FinallyCallbackType finally_callback);

 private:
  FinallyCallbackType finally_callback_;
  ContinuationImplType impl_;
};

/// A template for Continuation of void type. It has the same
/// interface as the generic version.
template <>
class CORE_API Continuation<void> final {
 public:
  /// Deleted `Continuation` constructor as an instance should not be created.
  Continuation() = delete;

  /**
   * @brief Adds an asynchronous task (one taking a callback) as the next
   * step of the  Continuation.
   *
   * @param task A task for adding to the continuation chain.
   *
   * @return The `Continuation` instance.
   */
  template <typename Callable>
  Continuation<internal::AsyncResultType<Callable>> Then(Callable task);

 private:
  /// An alias for `internal::ContinuationImpl::CallbackType`.
  using CallbackType = internal::ContinuationImpl::CallbackType;
  /// An alias for `internal::ContinuationImpl`.
  using ContinuationImplType = internal::ContinuationImpl;
  /// An alias for `ContinuationImplType::OutResultType`.
  using OutResultType = ContinuationImplType::OutResultType;
  /// An alias for `ContinuationImplType::TaskType`.
  using TaskType = ContinuationImplType::TaskType;

  template <typename ResultType>
  friend class Continuation;

  /**
   * @brief Creates the ContinuationImplType task based on parameters.
   *
   * @param context The `ExecutionContext` instance.
   * @param task Continuation task callback which constains an execution context
   * and returns a value for the next task.
   */
  template <typename NewType>
  static ContinuationImplType::ContinuationTask ToAsyncTask(
      ExecutionContext context,
      std::function<void(ExecutionContext, std::function<void(NewType)>)> task);

  ContinuationImplType impl_;
};

}  // namespace thread
}  // namespace olp

#include "Continuation.inl"
