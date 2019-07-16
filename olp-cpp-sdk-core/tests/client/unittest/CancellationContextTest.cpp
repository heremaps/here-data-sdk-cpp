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

#include <gtest/gtest.h>

#include <olp/core/client/CancellationContext.h>

using olp::client::CancellationContext;

TEST(CancellationContextTest, cancel_operation) {
  CancellationContext context;

  EXPECT_FALSE(context.IsCancelled());
  context.CancelOperation();
  EXPECT_TRUE(context.IsCancelled());
}

TEST(CancellationContextTest, copy_and_cancel_operation) {
  CancellationContext context;
  CancellationContext context_copy = context;

  EXPECT_FALSE(context.IsCancelled());
  EXPECT_FALSE(context_copy.IsCancelled());
  context.CancelOperation();
  EXPECT_TRUE(context.IsCancelled());
  EXPECT_TRUE(context_copy.IsCancelled());
}

TEST(CancellationContextTest, move_and_cancel_operation) {
  CancellationContext context;
  CancellationContext context_move = std::move(context);

  EXPECT_FALSE(context.IsCancelled());
  EXPECT_FALSE(context_move.IsCancelled());
  context_move.CancelOperation();
  EXPECT_FALSE(context.IsCancelled());
  EXPECT_TRUE(context_move.IsCancelled());
}
