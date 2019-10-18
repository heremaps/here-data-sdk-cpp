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

#include <olp/core/client/Condition.h>
#include "olp/core/client/ApiError.h"
#include "olp/core/client/ApiResponse.h"
#include "olp/core/client/CancellationContext.h"
#include "olp/core/client/CancellationToken.h"

namespace olp {
namespace dataservice {
namespace read {

class TaskContext {
 public:
  template <typename Exec, typename Callback>
  static TaskContext Create(Exec execute_func, Callback callback) {
    TaskContext context;
    context.SetExecutors(std::move(execute_func), std::move(callback));
    return context;
  }

  void Execute() const { impl_->Execute(); }

  void BlockingCancel(
      std::chrono::milliseconds timeout = std::chrono::seconds(60)) const {
    impl_->BlockingCancel(timeout);
  }

  client::CancellationToken CancelToken() const { return impl_->CancelToken(); }

  bool operator==(const TaskContext& other) const {
    return impl_ == other.impl_;
  }

 protected:
  friend struct TaskContextHash;

  TaskContext() = default;

  template <typename Exec, typename Callback,
            typename ExecResult = typename std::result_of<
                Exec(client::CancellationContext)>::type>
  void SetExecutors(Exec execute_func, Callback callback) {
    impl_ = std::make_shared<TaskContextImpl<typename ExecResult::ResultType>>(
        std::move(execute_func), std::move(callback));
  }

  class Impl {
   public:
    virtual ~Impl() = default;
    virtual void Execute() = 0;
    virtual void BlockingCancel(std::chrono::milliseconds timeout) = 0;
    virtual client::CancellationToken CancelToken() = 0;
  };

  template <typename T>
  class TaskContextImpl : public Impl {
   public:
    using Response = client::ApiResponse<T, client::ApiError>;
    using ExecuteFunc = std::function<Response(client::CancellationContext)>;
    using UserCallback = std::function<void(Response)>;

    TaskContextImpl(ExecuteFunc execute_func, UserCallback callback)
        : execute_func_(std::move(execute_func)),
          callback_(std::move(callback)),
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
        if (!context_.IsCancelled()) {
          user_response = std::move(response);
        }
      }

      if (callback) {
        callback(std::move(user_response));
      }

      condition_.Notify();

      state_.store(State::COMPLETED);
    }

    void BlockingCancel(std::chrono::milliseconds timeout) override {
      if (state_.load() == State::COMPLETED) {
        return;
      }

      // Cancel operation and wait for notification
      context_.CancelOperation();

      {
        std::lock_guard<std::mutex> lock(mutex_);
        execute_func_ = nullptr;
      }

      condition_.Wait(timeout);
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

struct TaskContextHash {
  size_t operator()(const TaskContext& task_context) const {
    return std::hash<std::shared_ptr<TaskContext::Impl>>()(task_context.impl_);
  }
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
