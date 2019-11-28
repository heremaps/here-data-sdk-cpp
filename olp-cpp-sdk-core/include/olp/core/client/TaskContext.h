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
#include <mutex>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/Condition.h>

namespace olp {
namespace client {

/**
 * @brief Generic type erased container, that encapsulates execution of
 * asynchronous task and invocation of callback in guaranteed manner. Once
 * result of provided task became available or error occurs, callback is
 * invoked.
 */
class CORE_API TaskContext {
 public:
  /**
   * @brief Constructs TaskContext with provided task and callback.
   * @param execute_func Task to be executed.
   * @param callback Is invoked once the result of execute_func is available or
   * task is cancelled.
   * @param context The CancellationContext to be used.
   * @return TaskContext that can be used to run or cancel corresponding task.
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
   * @brief Checks for cancellation, executes task and calls callback with
   * result or error.
   */
  void Execute() const { impl_->Execute(); }

  /**
   * @brief Cancel operation and wait for it finish.
   * @param timeout Milliseconds to wait on task finish.
   * @return False on timeout, True on notified wake.
   */
  bool BlockingCancel(
      std::chrono::milliseconds timeout = std::chrono::seconds(60)) const {
    return impl_->BlockingCancel(timeout);
  }

  /**
   * @brief Provides token to cancel task.
   * @return Token to cancel task.
   */
  client::CancellationToken CancelToken() const { return impl_->CancelToken(); }

  /// @brief Overload operator ==
  bool operator==(const TaskContext& other) const {
    return impl_ == other.impl_;
  }

 protected:
  friend struct TaskContextHash;

  TaskContext() = default;

  template <typename Exec, typename Callback,
            typename ExecResult = typename std::result_of<
                Exec(client::CancellationContext)>::type>
  void SetExecutors(Exec execute_func, Callback callback,
                    client::CancellationContext context) {
    impl_ = std::make_shared<TaskContextImpl<typename ExecResult::ResultType>>(
        std::move(execute_func), std::move(callback), std::move(context));
  }

  class Impl {
   public:
    virtual ~Impl() = default;

    virtual void Execute() = 0;

    virtual bool BlockingCancel(std::chrono::milliseconds timeout) = 0;

    virtual client::CancellationToken CancelToken() = 0;
  };

  template <typename T>
  class TaskContextImpl : public Impl {
   public:
    using Response = client::ApiResponse<T, client::ApiError>;
    using ExecuteFunc = std::function<Response(client::CancellationContext)>;
    using UserCallback = std::function<void(Response)>;

    TaskContextImpl(ExecuteFunc execute_func, UserCallback callback,
                    client::CancellationContext context)
        : execute_func_(std::move(execute_func)),
          callback_(std::move(callback)),
          context_(std::move(context)),
          state_{State::PENDING} {}

    ~TaskContextImpl() {}

    void Execute() override {
      State expected_state = State::PENDING;

      if (!state_.compare_exchange_strong(expected_state, State::IN_PROGRESS)) {
        return;
      }

      // Moving the user callback and function guarantee that they will be
      // executed exactly once
      ExecuteFunc function = nullptr;
      UserCallback callback = nullptr;

      {
        std::lock_guard<std::mutex> lock(mutex_);
        function = std::move(execute_func_);
        callback = std::move(callback_);
      }

      Response user_response =
          client::ApiError(client::ErrorCode::Cancelled, "Cancelled");

      if (function && !context_.IsCancelled()) {
        auto response = function(context_);
        // Cancel could occur during function execution, in that case we ignore
        // the response.
        if (!context_.IsCancelled() ||
            (!response.IsSuccessful() &&
             response.GetError().GetErrorCode() == ErrorCode::RequestTimeout)) {
          user_response = std::move(response);
        }
      }

      if (callback) {
        callback(std::move(user_response));
      }

      // Resources needs to be released before notification, else lambas would
      // have captured resources like network or TaskScheduler
      function = nullptr;
      callback = nullptr;

      condition_.Notify();

      state_.store(State::COMPLETED);
    }

    bool BlockingCancel(std::chrono::milliseconds timeout) override {
      if (state_.load() == State::COMPLETED) {
        return true;
      }

      // Cancel operation and wait for notification
      if (!context_.IsCancelled()) {
        context_.CancelOperation();
      }

      {
        std::lock_guard<std::mutex> lock(mutex_);
        execute_func_ = nullptr;
      }

      return condition_.Wait(timeout);
    }

    client::CancellationToken CancelToken() override {
      auto context = context_;
      return client::CancellationToken(
          [context]() mutable { context.CancelOperation(); });
    }

    enum class State { PENDING, IN_PROGRESS, COMPLETED };

    std::mutex mutex_;
    ExecuteFunc execute_func_;
    UserCallback callback_;
    client::CancellationContext context_;
    client::Condition condition_;
    std::atomic<State> state_;
  };

  std::shared_ptr<Impl> impl_;
};

/**
 * @brief Helper for unordered_* containers.
 */
struct TaskContextHash {
  size_t operator()(const TaskContext& task_context) const {
    return std::hash<std::shared_ptr<TaskContext::Impl>>()(task_context.impl_);
  }
};

}  // namespace client
}  // namespace olp
