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

#include <olp/core/client/Condition.h>

#include <gtest/gtest.h>
#include <future>

using olp::client::Condition;

namespace {
TEST(ConditionTest, NotifyBeforeWaitRespected) {
  Condition condition{};
  condition.Notify();
  EXPECT_TRUE(condition.Wait());
}

TEST(ConditionTest, WaitClearsTriggered) {
  Condition condition{};
  condition.Notify();
  EXPECT_TRUE(condition.Wait());
  EXPECT_FALSE(condition.Wait(std::chrono::seconds(0)));
}

TEST(ConditionTest, Timeouted) {
  Condition condition{};
  EXPECT_FALSE(condition.Wait(std::chrono::seconds(0)));
}

TEST(ConditionTest, WakeUp) {
  Condition condition{};
  auto wait_future =
      std::async(std::launch::async, [&]() { return condition.Wait(); });
  EXPECT_EQ(wait_future.wait_for(std::chrono::milliseconds(10)),
            std::future_status::timeout);
  condition.Notify();
  EXPECT_EQ(wait_future.wait_for(std::chrono::milliseconds(10)),
            std::future_status::ready);
  EXPECT_TRUE(wait_future.get());
}
}  // namespace
