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

#include <future>

#include <gtest/gtest.h>

#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/thread/Continuation.h>
#include <olp/core/thread/ExecutionContext.h>

namespace {
template <typename ResultType>
using Response = olp::client::ApiResponse<ResultType, olp::client::ApiError>;

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
  std::promise<int> promise;
  auto future = promise.get_future();

  auto continuation = Create([](olp::thread::ExecutionContext,
                                std::function<void(int)> next) { next(1); })
                          .Then([](olp::thread::ExecutionContext, int,
                                   std::function<void(int)> next) { next(2); })
                          .Finally([&promise](Response<int> response) {
                            promise.set_value(response.GetResult());
                          });
  continuation.Run();

  EXPECT_EQ(future.get(), 2);
}

TEST_F(ContinuationTest, CancelBeforeRun) {
  std::promise<olp::client::ApiError> promise;
  auto future = promise.get_future();

  auto continuation = Create([](olp::thread::ExecutionContext,
                                std::function<void(int)> next) { next(3); })
                          .Then([](olp::thread::ExecutionContext, int,
                                   std::function<void(int)> next) { next(3); })
                          .Finally([&promise](Response<int> response) {
                            promise.set_value(response.GetError());
                          });

  continuation.CancelToken().Cancel();
  continuation.Run();

  EXPECT_EQ(future.get().GetErrorCode(), olp::client::ErrorCode::Cancelled);
}

TEST_F(ContinuationTest, CancelAfterRun) {
  std::promise<olp::client::ApiError> promise;
  auto future = promise.get_future();

  auto continuation = Create([](olp::thread::ExecutionContext,
                                std::function<void(int)> next) { next(3); })
                          .Then([](olp::thread::ExecutionContext, int,
                                   std::function<void(int)> next) { next(3); })
                          .Finally([&promise](Response<int> response) {
                            promise.set_value(response.GetError());
                          });

  continuation.Run();
  continuation.CancelToken().Cancel();

  EXPECT_EQ(future.get().GetErrorCode(), olp::client::ErrorCode::Cancelled);
}

TEST_F(ContinuationTest, CancelExecution) {
  std::promise<void> promise;
  auto future = promise.get_future();

  olp::client::ApiResponse<olp::client::HttpResponse, olp::client::ApiError>
      result;

  auto cont =
      Create([](olp::thread::ExecutionContext, std::function<void(int)> next) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        next(1);
      })
          .Finally([&promise, &result](
                       olp::client::ApiResponse<int, olp::client::ApiError>
                           response) {
            result = response.GetError();
            promise.set_value();
          });

  cont.Run();
  std::this_thread::sleep_for(
      std::chrono::milliseconds(50));  // simulate some delay

  cont.CancelToken().Cancel();

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds(1000)),
            std::future_status::ready);

  future.get();

  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_EQ(result.GetError().GetHttpStatusCode(),
            static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR));
}

TEST_F(ContinuationTest, FinallyNotSet) {
  auto cont =
      Create([](olp::thread::ExecutionContext, std::function<void(int)>) {});

  cont.Run();
}

TEST_F(ContinuationTest, CancelWithFinallyNotSet) {
  auto cont =
      Create([](olp::thread::ExecutionContext, std::function<void(int)>) {});

  cont.CancelToken().Cancel();
  cont.Run();
}

TEST_F(ContinuationTest, Failed) {
  std::promise<olp::client::ApiError> api_error_promise;
  auto future = api_error_promise.get_future();

  auto cont = Create([](olp::thread::ExecutionContext context,
                        std::function<void(int)>) {
                context.SetError(olp::client::ApiError::NetworkConnection());
              }).Finally([&api_error_promise](Response<int> response) {
    api_error_promise.set_value(response.GetError());
  });

  cont.Run();
  auto result = future.get();

  EXPECT_EQ(result.GetErrorCode(), olp::client::ErrorCode::NetworkConnection);
}

TEST_F(ContinuationTest, NoCrashAfterCallingMethodsAfterRun) {
  std::promise<olp::client::ApiError> api_error_promise;
  auto future = api_error_promise.get_future();

  auto cont = Create([](olp::thread::ExecutionContext context,
                        std::function<void(int)>) {
                context.SetError(olp::client::ApiError::NetworkConnection());
              }).Finally([&api_error_promise](Response<int> response) {
    api_error_promise.set_value(response.GetError());
  });

  cont.Run();
  auto result = future.get();

  EXPECT_EQ(result.GetErrorCode(), olp::client::ErrorCode::NetworkConnection);

  cont.Then([](olp::thread::ExecutionContext, int, std::function<void(int)>) {
        FAIL();
      })
      .Finally([](Response<int>) { FAIL(); });
  cont.Run();
}

}  // namespace
