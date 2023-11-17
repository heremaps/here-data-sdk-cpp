/*
 * Copyright (C) 2023 HERE Europe B.V.
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

#include <olp/core/context/Context.h>

#include <gtest/gtest.h>

namespace {

using Context = olp::context::Context;
using Scope = olp::context::Context::Scope;

TEST(ContextTest, CallbacksAreInSingleton) {
  int init_counter = 0;
  int deinit_counter = 0;

  {
    SCOPED_TRACE("Second scope");

    Context::addInitializeCallbacks([&]() { ++init_counter; },
                                    [&]() { ++deinit_counter; });

    EXPECT_EQ(0, init_counter);
    EXPECT_EQ(0, deinit_counter);

    {
      Scope scope;
      EXPECT_EQ(1, init_counter);
      EXPECT_EQ(0, deinit_counter);
      {
        Scope second_scope;
        EXPECT_EQ(1, init_counter);
        EXPECT_EQ(0, deinit_counter);
      }
      EXPECT_EQ(1, init_counter);
      EXPECT_EQ(0, deinit_counter);
    }

    EXPECT_EQ(1, init_counter);
    EXPECT_EQ(1, deinit_counter);
  }

  {
    SCOPED_TRACE("Adding more callbacks");

    Context::addInitializeCallbacks([&]() { ++init_counter; },
                                    [&]() { ++deinit_counter; });

    EXPECT_EQ(1, init_counter);
    EXPECT_EQ(1, deinit_counter);

    {
      Scope scope;
      EXPECT_EQ(3, init_counter);
      EXPECT_EQ(1, deinit_counter);
    }

    EXPECT_EQ(3, init_counter);
    EXPECT_EQ(3, deinit_counter);
  }
}

}  // namespace
