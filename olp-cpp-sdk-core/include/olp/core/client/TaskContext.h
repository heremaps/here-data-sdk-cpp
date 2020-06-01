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

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/Condition.h>

namespace olp {
namespace client {

/**
 * @brief Encapsulates the execution of an asynchronous task and invocation of
 * a callback in a guaranteed manner.
 *
 * When the result of the provided task is available, or an error occurs,
 * the callback is invoked.
 */
class CORE_API TaskContext {
 public:
  /**
   * @brief Creates the `TaskContext` instance with the provided task and
   * callback.
   *
   * @param execute_func The task that should be executed.
   * @param callback Is invoked once the result of `execute_func` is available
   * or the task is cancelled.
   * @param context The `CancellationContext` instance.
   *
   * @return The `TaskContext` instance that can be used to run or cancel
   * the task.
   */
  template <typename Exec, typename Callback>
  static TaskContext Create(
      Exec execute_func, Callback callback,
      client::CancellationContext context = client::CancellationContext()) {
    TaskContext task;
    task.SetExecutors(std::move(execute_func), std::move(callback),
                      std::move(context));
    return task;
  }

  /**
   * @brief Checks for the cancellation, executes the task, and calls
   * the callback with the result or error.
   */
  void Execute() const { impl_->Execute(); }

  /**
   * @brief Cancels the operation and waits for the notification.
   *
   * @param timeout The time (in milliseconds) to wait for the task to finish.
   *
   * @return True if the notification is returned before the timeout; false
   * otherwise.
   */
  bool BlockingCancel(
      std::chrono::milliseconds timeout = std::chrono::seconds(60)) const {
    return impl_->BlockingCancel(timeout);
  }

  /**
   * @brief Provides a token to cancel the task.
   *
   * @return The `CancellationToken` instance.
   */
  client::CancellationToken CancelToken() const { return impl_->CancelToken(); }

  /**
   * @brief Checks whether the values of the `TaskContext` parameter are
   * the same as the values of the `other` parameter.
   *
   * @param other The `TaskContext` instance.
   *
   * @return True if the values of the `TaskContext` and `other` parameters are
   * equal; false otherwise.
   */
  bool operator==(const TaskContext& other) const {
    return impl_ == other.impl_;
  }

 protected:
  /// A helper for unordered containers.
  friend struct TaskContextHash;

  TaskContext() = default;

  template <typename Exec, typename Callback,
            typename ExecResult = typename std::result_of<
                Exec(client::CancellationContext)>::type>
  /**
   * @brief Sets the executors for the request.
   *
   * @param execute_func The task that should be executed.
   * @param callback Is invoked once the result of `execute_func` is available
   * or the task is cancelled.
   * @param context The `CancellationContext` instance.
   */
  void SetExecutors(Exec execute_func, Callback callback,
                    client::CancellationContext context) {
    impl_ = std::make_shared<TaskContextImpl<typename ExecResult::ResultType,
                                             typename ExecResult::ErrorType>>(
        std::move(execute_func), std::move(callback), std::move(context));
  }

  /**
   * @brief An implementation helper interface used to declare the `Execute`,
   * `BlockingCancel`, and `CancelToken` functions used by the `TaskContext`
   * instance.
   */
  class Impl {
   public:
    virtual ~Impl() = default;

    /**
     * @brief Checks for the cancellation, executes the task, and calls
     * the callback with the result or error.
     */
    virtual void Execute() = 0;

    /**
     * @brief Cancels the operation and waits for the notification.
     *
     * @param timeout The time (in milliseconds) to wait for the task to finish.
     *
     * @return True if the notification is returned before the timeout; false
     * otherwise.
     */
    virtual bool BlockingCancel(std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Provides a token to cancel the task.
     *
     * @return The `CancellationToken` instance.
     */
    virtual client::CancellationToken CancelToken() = 0;
  };

  /**
   * @brief Implements the `Impl` interface.
   *
   * Erases the type of the `Result` object produced by the `ExecuteFunc`
   * function and passes it to the `UserCallback` instance.
   *
   * @tparam T The result type.
   */
  template <typename T, typename ErrorType = client::ApiError>
  class TaskContextImpl : public Impl {
   public:
    /// Wraps the `T` typename in the API response.
    using Response = client::ApiResponse<T, ErrorType>;
    /// The task that produces the `Response` instance.
    using ExecuteFunc = std::function<Response(client::CancellationContext)>;
    /// Consumes the `Response` instance.
    using UserCallback = std::function<void(Response)>;

    /**
     * @brief Creates the `TaskContextImpl` instance.
     *
     * @param execute_func The task that should be executed.
     * @param callback Is invoked once the result of `execute_func` is available
     * or the task is cancelled.
     * @param context The `CancellationContext` instance.
     */
    TaskContextImpl(ExecuteFunc execute_func, UserCallback callback,
                    client::CancellationContext context)
        : execute_func_(std::move(execute_func)),
          callback_(std::move(callback)),
          context_(std::move(context)),
          state_{State::PENDING} {}

    ~TaskContextImpl() override{};

    /**
     * @brief Checks for the cancellation, executes the task, and calls
     * the callback with the result or error.
     */
    void Execute() override {
      State expected_state = State::PENDING;

      if (!state_.compare_exchange_strong(expected_state, State::IN_PROGRESS)) {
        return;
      }

      // Moving the user callback and function guarantee that they are
      // executed exactly once
      ExecuteFunc function = nullptr;
      UserCallback callback = nullptr;

      {
        std::lock_guard<std::mutex> lock(mutex_);
        function = std::move(execute_func_);
        callback = std::move(callback_);
      }

      Response user_response =
          ErrorType(client::ErrorCode::Cancelled, "Cancelled");

      if (function && !context_.IsCancelled()) {
        auto response = function(context_);
        // Cancel could occur during the function execution. In that case,
        // ignore the response.
        if (!context_.IsCancelled() ||
            (!response.IsSuccessful() &&
             response.GetError().GetErrorCode() == ErrorCode::RequestTimeout)) {
          user_response = std::move(response);
        }

        // Reset the context after the task is finished.
        context_ = CancellationContext();
      }

      if (callback) {
        callback(std::move(user_response));
      }

      // Resources need to be released before the notification, else lambas
      // would have captured resources like network or `TaskScheduler`.
      function = nullptr;
      callback = nullptr;

      condition_.Notify();
      state_.store(State::COMPLETED);
    }

    /**
     * @brief Cancels the operation and waits for the notification.
     *
     * @param timeout The time (in milliseconds) to wait for the task to finish.
     *
     * @return True if the notification is returned before the timeout; false
     * otherwise.
     */
    bool BlockingCancel(std::chrono::milliseconds timeout) override {
      if (state_.load() == State::COMPLETED) {
        return true;
      }

      // Cancels the operation and waits for the notification.
      if (!context_.IsCancelled()) {
        context_.CancelOperation();
      }

      {
        std::lock_guard<std::mutex> lock(mutex_);
        execute_func_ = nullptr;
      }

      return condition_.Wait(timeout);
    }

    /**
     * @brief Provides a token to cancel the task.
     *
     * @return The `CancellationToken` instance.
     */
    client::CancellationToken CancelToken() override {
      auto context = context_;
      return client::CancellationToken(
          [context]() mutable { context.CancelOperation(); });
    }

    /**
     * @brief Indicates the state of the request.
     */
    enum class State {
      /// The request waits to be executed.
      PENDING,
      /// The request is being executed.
      IN_PROGRESS,
      /// The request execution finished.
      COMPLETED
    };

    /// The mutex lock used to protect from the concurrent read and write
    /// operations.
    std::mutex mutex_;
    /// The `ExecuteFunc` instance.
    ExecuteFunc execute_func_;
    /// The `UserCallback` instance.
    UserCallback callback_;
    /// The `CancellationContext` instance.
    client::CancellationContext context_;
    /// The `Condition` instance.
    client::Condition condition_;
    /// The `State` enum of the atomic type.
    std::atomic<State> state_;
  };

  /// The `Impl` instance.
  std::shared_ptr<Impl> impl_;
};

/**
 * @brief A helper for unordered containers.
 */
struct CORE_API TaskContextHash {
  /**
   * @brief The hash function for the `TaskContext` instance.
   *
   * @param task_context The `TaskContext` instance.
   *
   * @return The hash for the `TaskContext` instance.
   */
  size_t operator()(const TaskContext& task_context) const {
    return std::hash<std::shared_ptr<TaskContext::Impl>>()(task_context.impl_);
  }
};

}  // namespace client
}  // namespace olp
