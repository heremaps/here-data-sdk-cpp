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

#include "TaskContext.h"

#include <gtest/gtest.h>

namespace {

using namespace olp::client;
using namespace olp::dataservice::read;

using ResponseType = std::string;
using Response = ApiResponse<ResponseType, ApiError>;
using ExecuteFunc = std::function<Response(CancellationContext)>;
using Callback = std::function<void(Response)>;

class TaskContextTestable : public TaskContext {
 public:
  std::function<void(void)> notify;

  template <typename Exec, typename Callback>
  static TaskContextTestable Create(Exec execute_func, Callback callback) {
    TaskContextTestable context;
    context.SetExecutors(std::move(execute_func), std::move(callback));
    return context;
  }

  template <typename Exec, typename Callback,
            typename ExecResult = typename std::result_of<
                Exec(olp::client::CancellationContext)>::type>
  void SetExecutors(Exec execute_func, Callback callback) {
    auto impl =
        std::make_shared<TaskContextImpl<typename ExecResult::ResultType>>(
            std::move(execute_func), std::move(callback));
    notify = [=]() { impl->condition_.Notify(); };
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
  std::mutex execution_mutex, cancelation_mutex;
  std::condition_variable execution_cv, cancelation_cv;

  auto execution_wait = [&]() {
    std::unique_lock<std::mutex> lock(execution_mutex);
    EXPECT_EQ(execution_cv.wait_for(lock, std::chrono::seconds(2)),
              std::cv_status::no_timeout);
  };

  auto cancelation_wait = [&]() {
    std::unique_lock<std::mutex> lock(cancelation_mutex);
    EXPECT_EQ(cancelation_cv.wait_for(lock, std::chrono::seconds(2)),
              std::cv_status::no_timeout);
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

  std::thread cancel_thread([&]() {
    cancelation_wait();
    context.BlockingCancel();
  });

  // wait threads to start and reach the task execution
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // reach cancel
  cancelation_cv.notify_one();
  // wait till execution starts
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // check for conditions and continue execution
  execution_cv.notify_one();

  execute_thread.join();
  cancel_thread.join();

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
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
  EXPECT_EQ(cancel_finished.wait_for(std::chrono::milliseconds(0)),
            std::future_status::ready);
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
}

TEST(TaskContextTest, CancelToken) {
  std::mutex execution_mutex;
  std::condition_variable execution_cv;

  auto execution_wait = [&]() {
    std::unique_lock<std::mutex> lock(execution_mutex);
    EXPECT_EQ(execution_cv.wait_for(lock, std::chrono::seconds(2)),
              std::cv_status::no_timeout);
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

  auto token = context.CancelToken();

  // wait till execution starts
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // cancel operation
  token.cancel();
  // check for conditions and continue execution
  execution_cv.notify_one();

  execute_thread.join();

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
}

}  // namespace
