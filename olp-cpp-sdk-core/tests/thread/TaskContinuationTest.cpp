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
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/thread/TaskContinuation.h>

const std::string BAD_REQUEST_MESSAGE = "Bad Request";

class TaskContinuationFixture : public ::testing::Test {
 public:
  void SetUp() override {
    scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  }

 protected:
  std::unique_ptr<olp::thread::TaskScheduler> scheduler;
};

TEST_F(TaskContinuationFixture, MultipleSequentialThen) {
  std::promise<void> promise;
  auto future = promise.get_future();

  olp::client::ApiResponse<int, olp::client::ApiError> result;

  auto cont =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Then([](olp::thread::ExecutionContext, int value,
                   std::function<void(int)> next) {
            EXPECT_EQ(value, 1);
            next(2);
          })
          .Then([](olp::thread::ExecutionContext, int value,
                   std::function<void(int)> next) {
            EXPECT_EQ(value, 2);
            next(3);
          })
          .Then([](olp::thread::ExecutionContext, int,
                   std::function<void(int)> next) { next(3); })
          .Finally([&promise, &result](
                       olp::client::ApiResponse<int, olp::client::ApiError>
                           response) {
            result = response.MoveResult();
            promise.set_value();
          });

  cont.Run();

  future.get();

  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_EQ(result.GetResult(), 3);
}

TEST_F(TaskContinuationFixture, FinallyNotSet) {
  auto cont = olp::thread::TaskContinuation(std::move(scheduler))
                  .Then([](olp::thread::ExecutionContext,
                           std::function<void(int)>) { FAIL(); });

  cont.Run();
}

TEST_F(TaskContinuationFixture, CancelBeforeRun) {
  std::promise<void> promise;
  auto future = promise.get_future();

  olp::client::ApiResponse<olp::client::HttpResponse, olp::client::ApiError>
      result;

  auto cont =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Finally([&promise, &result](
                       olp::client::ApiResponse<int, olp::client::ApiError>
                           response) {
            result = response.GetError();
            promise.set_value();
          });

  cont.CancelToken().Cancel();

  cont.Run();

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds(1000)),
            std::future_status::ready);

  future.get();

  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_EQ(result.GetError().GetHttpStatusCode(),
            static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR));
}

TEST_F(TaskContinuationFixture, CancelAfterRun) {
  std::promise<void> promise;
  auto future = promise.get_future();

  olp::client::ApiResponse<olp::client::HttpResponse, olp::client::ApiError>
      result;

  auto cont =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Then([](olp::thread::ExecutionContext, int,
                   std::function<void(olp::client::HttpResponse)> next) {
            next(olp::client::HttpResponse(200, "OK"));
          })
          .Finally([&promise,
                    &result](olp::client::ApiResponse<olp::client::HttpResponse,
                                                      olp::client::ApiError>
                                 response) {
            result = response.GetResult();
            promise.set_value();
          });

  cont.Run();
  std::this_thread::sleep_for(
      std::chrono::milliseconds(50));  // simulate some delay

  cont.CancelToken().Cancel();

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds(1000)),
            std::future_status::ready);

  future.get();

  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_EQ(result.GetError().GetHttpStatusCode(),
            static_cast<int>(olp::http::ErrorCode::UNKNOWN_ERROR));
}

TEST_F(TaskContinuationFixture, CallExecute) {
  std::promise<void> promise;
  auto future = promise.get_future();

  olp::client::ApiResponse<olp::client::HttpResponse, olp::client::ApiError>
      result;

  auto cont =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Then([](olp::thread::ExecutionContext context, int,
                   std::function<void(olp::client::HttpResponse)> next) {
            context.ExecuteOrCancelled([next]() {
              next(olp::client::HttpResponse(200, "OK"));
              return olp::client::CancellationToken();
            });
          })
          .Finally([&promise,
                    &result](olp::client::ApiResponse<olp::client::HttpResponse,
                                                      olp::client::ApiError>
                                 response) {
            result = response.GetResult();
            promise.set_value();
          });

  cont.Run();

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds(1000)),
            std::future_status::ready);

  future.get();

  EXPECT_TRUE(result.IsSuccessful());
}

TEST_F(TaskContinuationFixture, CallCancel) {
  std::promise<void> promise;
  auto future = promise.get_future();

  olp::client::ApiResponse<olp::client::HttpResponse, olp::client::ApiError>
      result;

  auto cont =
      olp::thread::TaskContinuation(std::move(scheduler))
          .Then([](olp::thread::ExecutionContext,
                   std::function<void(int)> next) { next(1); })
          .Then([](olp::thread::ExecutionContext context, int,
                   std::function<void(olp::client::HttpResponse)>) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            context.ExecuteOrCancelled(
                []() { return olp::client::CancellationToken(); },
                []() { return olp::client::CancellationToken(); });
          })
          .Finally([&promise,
                    &result](olp::client::ApiResponse<olp::client::HttpResponse,
                                                      olp::client::ApiError>
                                 response) {
            result = response.GetResult();
            promise.set_value();
          });

  cont.Run();
  std::this_thread::sleep_for(
      std::chrono::milliseconds(50));  // simulate some delay

  cont.CancelToken().Cancel();

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds(1000)),
            std::future_status::ready);

  future.get();

  EXPECT_TRUE(result.IsSuccessful());
}
