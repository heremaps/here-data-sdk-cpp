/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <olp/core/client/TaskContext.h>

#include <gtest/gtest.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/Condition.h>
#include <olp/core/thread/ThreadPoolTaskScheduler.h>

namespace {

namespace client = olp::client;
using client::ApiError;
using client::CancellationContext;
using client::CancellationToken;
using client::Condition;
using client::ErrorCode;
using client::TaskContext;
using ThreadPoolTaskScheduler = olp::thread::ThreadPoolTaskScheduler;

using ResponseType = std::string;
using Response = client::ApiResponse<ResponseType, client::ApiError>;
using ExecuteFunc = std::function<Response(CancellationContext)>;
using Callback = std::function<void(Response)>;

const auto kWaitTime = std::chrono::seconds(2);

class TaskContextTestable : public TaskContext {
 public:
  std::function<void(void)> notify;

  template <typename Exec, typename Callback>
  static TaskContextTestable Create(
      Exec execute_func, Callback callback,
      CancellationContext context = CancellationContext(),
      std::shared_ptr<olp::thread::TaskScheduler> task_scheduler = nullptr) {
    TaskContextTestable task;
    task.SetExecutors(std::move(execute_func), std::move(callback),
                      std::move(context), std::move(task_scheduler));
    return task;
  }

  template <typename Exec, typename Callback,
            typename ExecResult = typename std::result_of<
                Exec(olp::client::CancellationContext)>::type>
  void SetExecutors(
      Exec execute_func, Callback callback, CancellationContext context,
      std::shared_ptr<olp::thread::TaskScheduler> task_scheduler) {
    auto impl = std::make_shared<TaskContextImpl<ExecResult>>(
        std::move(execute_func), std::move(callback), context);
    std::weak_ptr<TaskContextImpl<ExecResult>> weak_impl = impl;
    auto cancellation_scheduler = task_scheduler;
    context.ExecuteOrCancelled(
        [weak_impl, cancellation_scheduler]() -> CancellationToken {
          return CancellationToken([weak_impl, cancellation_scheduler]() {
            auto impl = weak_impl.lock();
            if (impl && cancellation_scheduler) {
              cancellation_scheduler->ScheduleCancellationTask(
                  [weak_impl, cancellation_scheduler]() {
                    OLP_SDK_CORE_UNUSED(cancellation_scheduler);
                    if (auto impl = weak_impl.lock()) {
                      impl->PreExecuteCancel();
                    }
                  });
              return;
            }
            impl->PreExecuteCancel();
          });
        },
        []() {});
    notify = [impl]() { impl->condition_.Notify(); };
    impl_ = impl;
  }
};

TEST(TaskContextTest, ExecuteSimple) {
  int response_received = 0;

  ExecuteFunc func = [](CancellationContext) -> Response {
    return ApiError(ErrorCode::InvalidArgument, "test");
  };

  Response response;

  Callback callback = [&](Response r) -> void {
    response = std::move(r);
    response_received++;
  };

  {
    SCOPED_TRACE("Single execute");
    TaskContext context = TaskContext::Create(func, callback);
    context.Execute();
    EXPECT_EQ(response_received, 1);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::InvalidArgument);
  }
  response_received = 0;
  {
    SCOPED_TRACE("Multiple execute");
    TaskContext context = TaskContext::Create(func, callback);
    context.Execute();
    context.Execute();
    context.Execute();
    EXPECT_EQ(response_received, 1);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::InvalidArgument);
  }
  {
    SCOPED_TRACE("Cancel after execution");
    TaskContext context = TaskContext::Create(func, callback);
    context.Execute();
    context.BlockingCancel();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::InvalidArgument);
  }
  response_received = 0;
  {
    SCOPED_TRACE("Cancel before execution");

    TaskContext context = TaskContext::Create(func, callback);

    context.BlockingCancel(std::chrono::milliseconds{0});

    context.Execute();

    EXPECT_EQ(response_received, 1);
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  }
}

TEST(TaskContextTest, BlockingCancel) {
  Response response;

  Callback callback = [&](Response r) { response = std::move(r); };

  {
    SCOPED_TRACE("Pre-exec cancellation");
    bool executed = false;
    ExecuteFunc func = [&](CancellationContext) -> Response {
      executed = true;
      return std::string("Success");
    };

    TaskContext context = TaskContext::Create(func, callback);
    EXPECT_FALSE(context.BlockingCancel(std::chrono::seconds(0)));
    context.Execute();
    EXPECT_FALSE(executed);
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  }

  {
    SCOPED_TRACE("Cancel during execution");
    Condition continue_execution;
    Condition execution_started;
    int execution_count = 0;
    response = Response{};
    ExecuteFunc func = [&](CancellationContext c) -> Response {
      ++execution_count;
      execution_started.Notify();
      // EXPECT_TRUE(continue_execution.Wait(kWaitTime));
      const auto deadline = std::chrono::steady_clock::now() + kWaitTime;
      while (!c.IsCancelled() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
      }
      EXPECT_TRUE(c.IsCancelled());
      return std::string("Success");
    };
    TaskContext context = TaskContext::Create(func, callback);

    std::thread execute_thread([&]() { context.Execute(); });
    EXPECT_TRUE(execution_started.Wait());

    std::thread cancel_thread([&]() { EXPECT_TRUE(context.BlockingCancel()); });

    // continue_execution.Notify();

    execute_thread.join();
    cancel_thread.join();

    EXPECT_EQ(execution_count, 1);
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  }
}

TEST(TaskContextTest, PreExecuteCancelUsesCancellationLaneWhenEnabled) {
  auto task_scheduler = std::make_shared<ThreadPoolTaskScheduler>(1u, true);

  bool executed = false;
  Response response;
  std::promise<void> callback_promise;
  auto callback_future = callback_promise.get_future();
  std::thread::id callback_thread_id;
  std::promise<std::thread::id> executor_thread_id_promise;
  std::future<std::thread::id> executor_thread_id_future =
      executor_thread_id_promise.get_future();
  task_scheduler->ScheduleTask([&]() {
    executor_thread_id_promise.set_value(std::this_thread::get_id());
  });

  ASSERT_EQ(executor_thread_id_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);

  TaskContext context = TaskContext::Create(
      [&](CancellationContext) -> Response {
        executed = true;
        return std::string("Success");
      },
      [&](Response r) {
        callback_thread_id = std::this_thread::get_id();
        response = std::move(r);
        callback_promise.set_value();
      },
      CancellationContext(), task_scheduler);

  EXPECT_TRUE(context.BlockingCancel(kWaitTime));
  EXPECT_EQ(callback_future.wait_for(std::chrono::milliseconds(0)),
            std::future_status::ready);
  EXPECT_FALSE(executed);
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  EXPECT_NE(callback_thread_id, std::this_thread::get_id());
  EXPECT_NE(callback_thread_id, executor_thread_id_future.get());
}

TEST(TaskContextTest, PreExecuteCancelUsesRegularSchedulerWhenLaneDisabled) {
  auto task_scheduler = std::make_shared<ThreadPoolTaskScheduler>(1u, false);
  bool executed = false;
  Response response;
  std::promise<void> callback_promise;
  auto callback_future = callback_promise.get_future();
  std::thread::id callback_thread_id;

  std::promise<std::thread::id> executor_thread_id_promise;
  std::future<std::thread::id> executor_thread_id_future =
      executor_thread_id_promise.get_future();
  task_scheduler->ScheduleTask([&]() {
    executor_thread_id_promise.set_value(std::this_thread::get_id());
  });

  ASSERT_EQ(executor_thread_id_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);

  TaskContext context = TaskContext::Create(
      [&](CancellationContext) -> Response {
        executed = true;
        return std::string("Success");
      },
      [&](Response r) {
        callback_thread_id = std::this_thread::get_id();
        response = std::move(r);
        callback_promise.set_value();
      },
      CancellationContext(), task_scheduler);

  EXPECT_TRUE(context.BlockingCancel(kWaitTime));
  EXPECT_EQ(callback_future.wait_for(std::chrono::milliseconds(0)),
            std::future_status::ready);
  EXPECT_FALSE(executed);
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  EXPECT_NE(callback_thread_id, std::this_thread::get_id());
  EXPECT_EQ(callback_thread_id, executor_thread_id_future.get());
}

TEST(TaskContextTest,
     CancellationLanePreExecuteCancelRunsWhileRegularWorkerBlocked) {
  auto scheduler = std::make_shared<ThreadPoolTaskScheduler>(1u, true);
  std::promise<Response> response;
  std::promise<void> scheduler_blocked_promise;

  scheduler->ScheduleTask([&]() {
    scheduler_blocked_promise.set_value();
    auto future = response.get_future();
    ASSERT_EQ(future.wait_for(std::chrono::seconds(1)),
              std::future_status::ready);

    EXPECT_EQ(future.get().GetError().GetErrorCode(), ErrorCode::Cancelled);
  });

  ASSERT_EQ(
      scheduler_blocked_promise.get_future().wait_for(std::chrono::seconds(1)),
      std::future_status::ready);

  TaskContext context = TaskContext::Create(
      [&](CancellationContext) -> Response { return std::string("Success"); },
      [&](Response r) { response.set_value(std::move(r)); },
      CancellationContext(), scheduler);

  ASSERT_TRUE(context.BlockingCancel(kWaitTime));
}

TEST(TaskContextTest, BlockingCancelIsWaiting) {
  std::future<void> cancel_finished;
  std::function<void()> release_wait;
  int response_received = 0;

  ExecuteFunc func = [&](CancellationContext) -> Response {
    EXPECT_EQ(cancel_finished.wait_for(std::chrono::milliseconds(0)),
              std::future_status::timeout);
    release_wait();
    cancel_finished.wait();
    return ApiError(ErrorCode::InvalidArgument, "test");
  };

  Response response;

  Callback callback = [&](Response r) {
    response = std::move(r);
    response_received++;
  };

  TaskContextTestable context = TaskContextTestable::Create(func, callback);

  cancel_finished =
      std::async(std::launch::async, [&]() { context.BlockingCancel(); });

  release_wait = [&]() { context.notify(); };

  context.Execute();

  EXPECT_EQ(response_received, 1);
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(cancel_finished.wait_for(std::chrono::milliseconds(10)),
            std::future_status::ready);
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
}

TEST(TaskContextTest, CancelToken) {
  Condition continue_execution;
  Condition execution_started;

  auto execution_wait = [&]() {
    execution_started.Notify();
    EXPECT_TRUE(continue_execution.Wait(kWaitTime));
  };

  ExecuteFunc func = [&](CancellationContext c) -> Response {
    execution_wait();
    EXPECT_TRUE(c.IsCancelled());
    return std::string("Success");
  };

  Response response;

  Callback callback = [&](Response r) { response = std::move(r); };

  TaskContext context = TaskContext::Create(func, callback);

  std::thread execute_thread([&]() { context.Execute(); });

  EXPECT_TRUE(execution_started.Wait());

  context.CancelToken().Cancel();

  continue_execution.Notify();

  execute_thread.join();

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
}

TEST(TaskContextTest, LateCancel) {
  ExecuteFunc func = [&](CancellationContext c) -> Response {
    // Late cancel should not trigger the cancellation function.
    c.ExecuteOrCancelled([]() -> CancellationToken {
      return CancellationToken([]() { FAIL(); });
    });
    return std::string("Success");
  };

  Response response;

  Callback callback = [&](Response r) { response = std::move(r); };

  CancellationToken token;

  {
    TaskContext context = TaskContext::Create(func, callback);
    token = context.CancelToken();
    context.Execute();
  }

  token.Cancel();
}

TEST(TaskContextTest, OLPSUP_10456) {
  // Cancel should not be triggered from the inside of Execute function.
  // This happens when the execution function is keeping the last owning
  // reference to the object that cancels the task in the destructor.
  struct TaskWrapper {
    ~TaskWrapper() { context->BlockingCancel(std::chrono::milliseconds(0)); }
    std::shared_ptr<TaskContext> context;
  };

  auto wrapper = std::make_shared<TaskWrapper>();

  bool cancel_triggered = false;
  auto cancel_function = [&]() { cancel_triggered = true; };

  ExecuteFunc execute_func = [&, wrapper](CancellationContext c) -> Response {
    c.ExecuteOrCancelled([&]() { return CancellationToken(cancel_function); });
    return std::string("Success");
  };

  Response response;
  Callback callback = [&](Response r) { response = std::move(r); };

  // Initialize the task context
  wrapper->context = std::make_shared<TaskContext>(
      TaskContext::Create(std::move(execute_func), std::move(callback)));

  TaskContext task_context = *(wrapper->context).get();

  // Now execute_func is the only owner of task context via TaskWrapper.
  wrapper.reset();

  // Execute the execute_func,
  task_context.Execute();

  EXPECT_EQ(response.GetResult(), "Success");
  EXPECT_FALSE(cancel_triggered);
}

}  // namespace
