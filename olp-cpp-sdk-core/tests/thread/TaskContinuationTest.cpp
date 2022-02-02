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
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/http/NetworkUtils.h>
#include <olp/core/thread/TaskContinuation.h>
#include <olp/core/thread/ThreadPoolTaskScheduler.h>

namespace {
constexpr auto kMaxWaitMs = std::chrono::milliseconds(100);
constexpr auto kTimeoutMs = std::chrono::milliseconds(50);

template <typename Response>
using ResponseType = olp::client::ApiResponse<Response, olp::client::ApiError>;

class TaskContinuationFixture : public ::testing::Test {
 public:
  void SetUp() override {
    scheduler = std::make_shared<olp::thread::ThreadPoolTaskScheduler>();
  }

 protected:
  std::shared_ptr<olp::thread::TaskScheduler> scheduler;
};

TEST_F(TaskContinuationFixture, MultipleSequentialThen) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  int counter = 0;
  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([&](olp::thread::ExecutionContext,
                    std::function<void(int)> next) { next(++counter); })
          .Then([&](olp::thread::ExecutionContext, int value,
                    std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            next(counter);
          })
          .Then([&](olp::thread::ExecutionContext, int value,
                    std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
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
  ASSERT_EQ(result.GetResult(), counter);
}

TEST_F(TaskContinuationFixture, FinallyNotSet) {
  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext, std::function<void(int)>) {
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

TEST_F(TaskContinuationFixture, CancelBeforeRun) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  auto continuation = olp::thread::TaskContinuation(std::move(scheduler))
                          .Then([](olp::thread::ExecutionContext,
                                   std::function<void(int)> next) { next(1); })
                          .Finally([&](ResponseType<int> response) {
                            promise.set_value(std::move(response));
                          });

  continuation.CancelToken().Cancel();

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  ASSERT_EQ(result.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(TaskContinuationFixture, CancelAfterRun) {
  std::promise<ResponseType<olp::client::HttpResponse>> promise;
  auto future = promise.get_future();

  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Then([](olp::thread::ExecutionContext, int,
                   std::function<void(olp::client::HttpResponse)> next) {
            next(olp::client::HttpResponse(
                olp::http::HttpStatusCode::OK,
                olp::http::HttpErrorToString(olp::http::HttpStatusCode::OK)));
          })
          .Finally([&](ResponseType<olp::client::HttpResponse> response) {
            promise.set_value(std::move(response));
          });

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();
  continuation.CancelToken().Cancel();

  EXPECT_TRUE(result);
  ASSERT_EQ(result.GetError().GetHttpStatusCode(),
            static_cast<int>(olp::http::ErrorCode::UNKNOWN_ERROR));
}

TEST_F(TaskContinuationFixture, CallExecute) {
  std::promise<ResponseType<olp::client::HttpResponse>> promise;
  auto future = promise.get_future();

  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Then([](olp::thread::ExecutionContext context, int,
                   std::function<void(olp::client::HttpResponse)> next) {
            context.ExecuteOrCancelled([=]() {
              next(olp::client::HttpResponse(
                  olp::http::HttpStatusCode::OK,
                  olp::http::HttpErrorToString(olp::http::HttpStatusCode::OK)));
              return olp::client::CancellationToken();
            });
          })
          .Finally([&](ResponseType<olp::client::HttpResponse> response) {
            promise.set_value(std::move(response));
          });

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_TRUE(result);
  ASSERT_EQ(result.GetResult().GetStatus(), olp::http::HttpStatusCode::OK);
}

TEST_F(TaskContinuationFixture, CallCancel1) {
  std::promise<ResponseType<olp::client::HttpResponse>> promise;
  auto future = promise.get_future();

  std::promise<void> cancel_promise;
  auto cancel_future = cancel_promise.get_future();

  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Then([&](olp::thread::ExecutionContext context, int,
                    std::function<void(olp::client::HttpResponse)>) {
            ASSERT_EQ(cancel_future.wait_for(kMaxWaitMs),
                      std::future_status::ready);
            context.ExecuteOrCancelled(
                []() { return olp::client::CancellationToken(); },
                []() { return olp::client::CancellationToken(); });
          })
          .Finally([&](ResponseType<olp::client::HttpResponse> response) {
            promise.set_value(std::move(response));
          });

  continuation.Run();

  continuation.CancelToken().Cancel();
  cancel_promise.set_value();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  ASSERT_EQ(result.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(TaskContinuationFixture, MultipleSequentialThenAsync) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  int counter = 0;
  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([&](olp::thread::ExecutionContext,
                    std::function<void(int)> next) {
            std::thread([=, &counter]() { next(++counter); }).detach();
          })
          .Then([&](olp::thread::ExecutionContext, int value,
                    std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            next(++counter);
          })
          .Then([&](olp::thread::ExecutionContext, int value,
                    std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            std::thread([=, &counter]() {
              std::thread([=, &counter]() { next(++counter); }).detach();
            }).detach();
          })
          .Then([&](olp::thread::ExecutionContext, int value,
                    std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            next(++counter);
          })
          .Finally([&](ResponseType<int> response) {
            promise.set_value(std::move(response));
          });

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_TRUE(result);
  ASSERT_EQ(result.GetResult(), counter);
}

TEST_F(TaskContinuationFixture, CallExecuteAsync) {
  std::promise<ResponseType<olp::client::HttpResponse>> promise;
  auto future = promise.get_future();

  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then(
              [](olp::thread::ExecutionContext, std::function<void(int)> next) {
                std::thread([=]() { next(1); }).detach();
              })
          .Then([](olp::thread::ExecutionContext context, int,
                   std::function<void(olp::client::HttpResponse)> next) {
            context.ExecuteOrCancelled([next]() {
              std::thread([=]() {
                next(olp::client::HttpResponse(
                    olp::http::HttpStatusCode::CREATED,
                    olp::http::HttpErrorToString(
                        olp::http::HttpStatusCode::CREATED)));
              }).detach();
              return olp::client::CancellationToken();
            });
          })
          .Finally([&](ResponseType<olp::client::HttpResponse> response) {
            promise.set_value(std::move(response));
          });

  continuation.Run();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_TRUE(result);
  ASSERT_EQ(result.GetResult().GetStatus(), olp::http::HttpStatusCode::CREATED);
}

TEST_F(TaskContinuationFixture, CheckNoDeadlockSingleThread) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  int counter = 0;
  auto continuation =
      olp::thread::TaskContinuation(scheduler)
          .Then([&counter](olp::thread::ExecutionContext,
                           std::function<void(int)> next) { next(++counter); })
          .Then([this, &counter](olp::thread::ExecutionContext, int value,
                                 std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            scheduler->ScheduleTask([=, &counter]() { next(++counter); });
          })
          .Then([this, &counter](olp::thread::ExecutionContext, int value,
                                 std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            scheduler->ScheduleTask([=, &counter]() { next(++counter); });
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
  ASSERT_EQ(result.GetResult(), counter);
}

TEST_F(TaskContinuationFixture, CheckNoDeadlockMultipleThreads) {
  scheduler = std::make_shared<olp::thread::ThreadPoolTaskScheduler>(2);

  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  int counter = 0;
  auto continuation =
      olp::thread::TaskContinuation(scheduler)
          .Then([&](olp::thread::ExecutionContext,
                    std::function<void(int)> next) { next(++counter); })
          .Then([this, &counter](olp::thread::ExecutionContext, int value,
                                 std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            scheduler->ScheduleTask([=, &counter]() { next(++counter); });
          })
          .Then([this, &counter](olp::thread::ExecutionContext, int value,
                                 std::function<void(int)> next) {
            ASSERT_EQ(value, counter);
            scheduler->ScheduleTask([=, &counter]() { next(++counter); });
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
  ASSERT_EQ(result.GetResult(), counter);
}

TEST_F(TaskContinuationFixture, FailedAsync) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  std::promise<void> cancel_promise;
  auto cancel_future = cancel_promise.get_future();

  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([&](olp::thread::ExecutionContext context,
                    std::function<void(int)>) {
            std::thread([context, &cancel_future]() mutable {
              ASSERT_EQ(cancel_future.wait_for(kMaxWaitMs),
                        std::future_status::ready);
              context.SetError(olp::client::ApiError::NetworkConnection());
            }).detach();
          })
          .Finally([&](ResponseType<int> response) {
            promise.set_value(std::move(response));
          });

  continuation.Run();
  continuation.CancelToken().Cancel();
  cancel_promise.set_value();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  EXPECT_EQ(result.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(TaskContinuationFixture, CancelAsync) {
  std::promise<ResponseType<int>> promise;
  auto future = promise.get_future();

  std::promise<void> cancel_promise;
  auto cancel_future = cancel_promise.get_future();

  auto continuation =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([&](olp::thread::ExecutionContext,
                    std::function<void(int)> next) {
            ASSERT_EQ(cancel_future.wait_for(kMaxWaitMs),
                      std::future_status::ready);
            next(1);
          })
          .Then(
              [](olp::thread::ExecutionContext, int, std::function<void(int)>) {
                FAIL() << "The second `Then` method should not be called";
              })
          .Finally([&](ResponseType<int> response) {
            promise.set_value(std::move(response));
          });

  continuation.Run();
  continuation.CancelToken().Cancel();
  cancel_promise.set_value();

  ASSERT_EQ(future.wait_for(kMaxWaitMs), std::future_status::ready);
  const auto result = future.get();

  EXPECT_FALSE(result);
  ASSERT_EQ(result.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}
}  // namespace
