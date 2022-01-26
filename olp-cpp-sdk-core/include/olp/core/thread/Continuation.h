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

/**
 * @brief Provides mechanisms to create a chain of tasks and start, cancel,
 * and finalize an execution.
 *
 * @note It is a private implementation class for internal use only and not
 * bound to any API stability promises. Do not use it directly.
 */
class CORE_API ContinuationImpl final {
 public:
  /// An alias for the function that returns an error as a callback.
  using FailedCallback = std::function<void(client::ApiError)>;
  /// The return value type of the `Continuation` task.
  using OutResultType = std::unique_ptr<UntypedSmartPointer>;
  /// The type of `ContinuationType`.
  using TaskType = std::function<OutResultType(void*)>;
  /// The generic callback type.
  using CallbackType = std::function<void(void*)>;
  /// An internal type of tasks in `Continuation`.
  using AsyncTaskType = std::function<void(void*, CallbackType)>;
  /// An alias for the processing tasks finalization type.
  using FinalCallbackType = std::function<void(void*, bool)>;
  /// An alias for a pair of task continuation chain types.
  using ContinuationTask = std::pair<AsyncTaskType, TaskType>;

  /// The default constructor of `ContinuationImpl`.
  ContinuationImpl() = default;

  /**
   * @brief Creates the `ContinuationImpl` instance.
   *
   * @param task_scheduler The `TaskScheduler` instance.
   * @param context The `ExecutionContext` instance.
   * @param task The `ContinuationTask` instance. It represents
   * a task that you want to add to the continuation chain.
   */
  ContinuationImpl(std::shared_ptr<TaskScheduler> task_scheduler,
                   ExecutionContext context, ContinuationTask task);

  /**
   * @brief Adds the next asynchronous task
   * to the `ContinuationImpl` instance.
   *
   * @param task The `ContinuationTask` instance. It represents
   * a task that you want to add to the continuation chain.
   *
   * @return The `ContinuationImpl` instance.
   */
  ContinuationImpl Then(ContinuationTask task);

  /**
   * @brief Starts the execution of the task continuation chain.
   *
   * @param callback Handles the finalization of the task chain.
   */
  void Run(FinalCallbackType callback);

  /**
   * @brief Gets the `ExecutionContext` object.
   *
   * @return The `ExecutionContext` instance.
   */
  const ExecutionContext& GetExecutionContext() const;

  /**
   * @brief Gets the `CancellationContext` object.
   *
   * @return The `CancellationContext` instance.
   */
  const client::CancellationContext& GetContext() const;

  /**
   * @brief Checks whether the `CancellationContext` instance is cancelled.
   *
   * @return True if the `CancellationContext` instance is cancelled; false
   * otherwise.
   */
  bool Cancelled() const;

  /**
   * @brief Sets a callback on calling `SetError`.
   *
   * @param callback Handles the execution finalization of the continuation
   * chain.
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

  /**
   * @brief Checks whether it is allowed to do the changes to the task chain.
   *
   * Use it to skip the methods of the `Continuation` object.
   */
  bool change_allowed{true};
};

}  // namespace internal

/// A generic template for `Continuation`.
template <typename ResultType>
class CORE_API Continuation final {
  /// The response type alias of the template class type.
  using Response = client::ApiResponse<ResultType, client::ApiError>;
  /// An alias for the finalization callback.
  using FinallyCallbackType = std::function<void(Response)>;
  /// An alias for `ContinuationImplType::ContinuationImpl`.
  using ContinuationImplType = internal::ContinuationImpl;
  /// An alias for `ContinuationImplType::OutResultType`.
  using OutResultType = ContinuationImplType::OutResultType;
  /// An alias for `ContinuationImplType::TaskType`.
  using TaskType = ContinuationImplType::TaskType;
  /// An alias for `ContinuationImplType::CallbackType`.
  using CallbackType = ContinuationImplType::CallbackType;
  /// An alias for the continuation chain first task.
  using ContinuationTaskType =
      std::function<void(ExecutionContext, std::function<void(ResultType)>)>;

 public:
  /// The default constructor of `Continuation<ResultType>`.
  Continuation() = default;

  /**
   * @brief Creates the `Continuation` instance.
   *
   * @param task_scheduler The `TaskScheduler` instance.
   * @param context The `ExecutionContext` instance.
   * @param task The continuation task's callback. It contains the execution
   * context and returns a value for the next task.
   */
  Continuation(std::shared_ptr<thread::TaskScheduler> scheduler,
               ExecutionContext context, ContinuationTaskType task);

  /**
   * @brief Creates the `Continuation` instance.
   *
   * @param continuation The `ContinuationImpl` instance.
   */
  Continuation(ContinuationImplType continuation);

  /**
   * @brief Adds the next asynchronous task
   * to the `ContinuationImpl` instance.
   *
   * @param task The `ContinuationTask` instance. It represents
   * a task that you want to add to the continuation chain.
   */
  template <typename Callable>
  Continuation<internal::DeducedType<Callable>> Then(Callable task);

  /// Starts the execution of the task continuation chain.
  void Run();

  /**
   * @brief Provides a token to cancel the task.
   *
   * @return The `CancellationToken` instance.
   */
  client::CancellationToken CancelToken();

  /**
   * @brief Adds the next asynchronous task
   * to the `ContinuationImpl` instance.
   *
   * @param execution_func A task that you want to add to the continuation
   * chain.
   */
  template <typename NewType>
  Continuation<internal::RemoveRefAndConst<NewType>> Then(
      std::function<void(ExecutionContext, ResultType,
                         std::function<void(NewType)>)>
          execution_func);

  /**
   * @brief Handles the finalization of the `Continuation` instance
   * and sets a callback for it.
   *
   * @param finally_callback The callback that handles successful and
   * unsuccessful results.
   *
   * @return The `Continuation` instance.
   */
  Continuation& Finally(FinallyCallbackType finally_callback);

 private:
  FinallyCallbackType finally_callback_;
  ContinuationImplType impl_;
};

/**
 * @brief A template for `Continuation` of the void type.
 *
 * It has the same interface as the generic version.
 */
template <>
class CORE_API Continuation<void> final {
 public:
  /// Do not create the deleted `Continuation` constructor as an instance.
  Continuation() = delete;

  /**
   * @brief Adds the next asynchronous task
   * to the `ContinuationImpl` instance.
   *
   * @param task The `ContinuationTask` instance. It represents
   * a task that you want to add to the continuation chain.
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
   * @brief Creates the `ContinuationImplType` instance.
   *
   * @param context The `ExecutionContext` instance.
   * @param task The continuation task's callback. It contains the execution
   * context and returns a value for the next task.
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
