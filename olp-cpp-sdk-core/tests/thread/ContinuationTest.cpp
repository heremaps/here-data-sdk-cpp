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

#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/thread/Continuation.h>
#include <olp/core/thread/ExecutionContext.h>

namespace {
constexpr auto kMaxWaitMs = std::chrono::milliseconds(100);
constexpr auto kTimeoutMs = std::chrono::milliseconds(50);

template <typename ResultType>
using ResponseType =
    olp::client::ApiResponse<ResultType, olp::client::ApiError>;

class ContinuationTest : public testing::Test {
 public:
  void SetUp() override {
    task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  }

 protected:
  template <typename Callable>
  olp::thread::Continuation<olp::thread::internal::AsyncResultType<Callable>>
  Create(Callable func) {
    using NewResultType = olp::thread::internal::AsyncResultType<Callable>;

    using Function = olp::thread::internal::TypeToFunctionInput<NewResultType>;
    return {std::move(task_scheduler), std::move(execution_context),
            std::function<void(olp::thread::ExecutionContext, Function)>(
                std::move(func))};
  }

  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler;
  olp::thread::ExecutionContext execution_context;
};

TEST_F(ContinuationTest, MultipleThen) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  int counter = 0;
  auto continuation =
      Create([&](olp::thread::ExecutionContext, std::function<void(int)> next) {
        next(++counter);
      })
          .Then([&](olp::thread::ExecutionContext, int,
                    std::function<void(int)> next) { next(++counter); })
          .Finally([&](ResponseType<int> response) {
            promise.set_value(std::move(response));
          });
  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_TRUE(result);
  EXPECT_EQ(result.GetResult(), counter);
}

TEST_F(ContinuationTest, CancelBeforeRun) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  auto continuation = Create([](olp::thread::ExecutionContext,
                                std::function<void(int)> next) { next(3); })
                          .Then([](olp::thread::ExecutionContext, int,
                                   std::function<void(int)> next) { next(3); })
                          .Finally([&](ResponseType<int> response) {
                            promise.set_value(std::move(response));
                          });

  continuation.CancelToken().Cancel();
  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  EXPECT_EQ(result.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(ContinuationTest, CancelAfterRun) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  auto continuation = Create([](olp::thread::ExecutionContext,
                                std::function<void(int)> next) { next(3); })
                          .Then([](olp::thread::ExecutionContext, int,
                                   std::function<void(int)> next) { next(3); })
                          .Finally([&](ResponseType<int> response) {
                            promise.set_value(std::move(response));
                          });

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  continuation.CancelToken().Cancel();

  EXPECT_TRUE(result);
}

TEST_F(ContinuationTest, CancelExecution) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  std::promise<void> cancel_promise;
  auto cancel_future = cancel_promise.get_future();

  auto continuation =
      Create([&](olp::thread::ExecutionContext, std::function<void(int)> next) {
        ASSERT_EQ(cancel_future.wait_for(kMaxWaitMs),
                  std::future_status::ready);
        next(1);
      }).Finally([&](ResponseType<int> response) {
        promise.set_value(std::move(response));
      });

  continuation.Run();

  continuation.CancelToken().Cancel();
  cancel_promise.set_value();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  EXPECT_EQ(result.GetError().GetHttpStatusCode(),
            static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR));
}

TEST_F(ContinuationTest, FinallyNotSet) {
  auto continuation =
      Create([](olp::thread::ExecutionContext, std::function<void(int)>) {
        FAIL() << "`Then` method should not be called if the `Finally`"
                  "callback isn't set";
      });

  continuation.Run();
  // Wait some time to enshure no callbacks are called during the async `Run()`
  // call. Otherwise it is possible the callbacks will be called after current
  // test finishes, which may lead to crashes and ambiguity which test caused an
  // issue.
  std::this_thread::sleep_for(kTimeoutMs);
}

TEST_F(ContinuationTest, CancelWithFinallyNotSet) {
  auto continuation =
      Create([](olp::thread::ExecutionContext, std::function<void(int)>) {
        FAIL() << "`Then` method should not be called if the `Finally`"
                  "callback isn't set";
      });

  continuation.CancelToken().Cancel();
  continuation.Run();
  // Wait some time to enshure no callbacks are called during the async `Run()`
  // call. Otherwise it is possible the callbacks will be called after current
  // test finishes, which may lead to crashes and ambiguity which test caused an
  // issue.
  std::this_thread::sleep_for(kTimeoutMs);
}

TEST_F(ContinuationTest, Failed) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  auto continuation =
      Create([](olp::thread::ExecutionContext context,
                std::function<void(int)>) {
        context.SetError(olp::client::ApiError::NetworkConnection());
      }).Finally([&](ResponseType<int> response) {
        promise.set_value(std::move(response));
      });

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  ASSERT_EQ(result.GetError().GetErrorCode(),
            olp::client::ErrorCode::NetworkConnection);
}

TEST_F(ContinuationTest, NoCrashAfterCallingMethodsAfterRun) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  auto continuation =
      Create([](olp::thread::ExecutionContext context,
                std::function<void(int)>) {
        context.SetError(olp::client::ApiError::NetworkConnection());
      }).Finally([&](ResponseType<int> response) {
        promise.set_value(std::move(response));
      });

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  ASSERT_EQ(result.GetError().GetErrorCode(),
            olp::client::ErrorCode::NetworkConnection);

  continuation
      .Then([](olp::thread::ExecutionContext, int, std::function<void(int)>) {
        FAIL() << "`Then` method should not be called after the continuation "
                  "is already run";
      })
      .Finally([](ResponseType<int>) {
        FAIL() << "`Finally` method should not be called after the "
                  "continuation is already run";
      });
  continuation.Run();
  // Wait some time to enshure no callbacks are called during the async `Run()`
  // call. Otherwise it is possible the callbacks will be called after current
  // test finishes, which may lead to crashes and ambiguity which test caused an
  // issue.
  std::this_thread::sleep_for(kTimeoutMs);
}

}  // namespace
