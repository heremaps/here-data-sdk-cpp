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

#include <gtest/gtest.h>

#include <olp/core/thread/ExecutionContext.h>

namespace {
using ExecutionContext = olp::thread::ExecutionContext;

TEST(ExecutionContextTest, Cancel) {
  ExecutionContext execution_context;

  EXPECT_FALSE(execution_context.Cancelled());
  execution_context.CancelOperation();
  EXPECT_TRUE(execution_context.Cancelled());
}

TEST(ExecutionContextTest, ExecuteOrCancelled) {
  ExecutionContext execution_context;

  {
    SCOPED_TRACE("Execute");

    bool executed = false;
    bool cancelled = false;
    execution_context.ExecuteOrCancelled(
        [&executed]() {
          executed = true;
          return olp::client::CancellationToken();
        },
        [&cancelled]() { cancelled = true; });

    EXPECT_TRUE(executed);
    EXPECT_FALSE(cancelled);
  }

  {
    SCOPED_TRACE("Cancel");

    bool executed = false;
    bool cancelled = false;
    execution_context.CancelOperation();
    execution_context.ExecuteOrCancelled(
        [&]() {
          executed = true;
          return olp::client::CancellationToken();
        },
        [&cancelled]() { cancelled = true; });

    EXPECT_FALSE(executed);
    EXPECT_TRUE(cancelled);
  }
}

TEST(ExecutionContextTest, SetFailedCallback) {
  ExecutionContext execution_context;

  olp::client::ApiError api_error;
  execution_context.SetFailedCallback(
      [&](olp::client::ApiError error) { api_error = std::move(error); });
  execution_context.SetError(olp::client::ApiError::NetworkConnection());

  EXPECT_EQ(api_error.GetErrorCode(),
            olp::client::ErrorCode::NetworkConnection);
}

TEST(ExecutionContextTest, GetContext) {
  ExecutionContext execution_context;

  EXPECT_FALSE(execution_context.GetContext().IsCancelled());
  execution_context.CancelOperation();
  EXPECT_TRUE(execution_context.GetContext().IsCancelled());
}

}  // namespace
