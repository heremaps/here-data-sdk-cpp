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

#include <gtest/gtest.h>

#include <olp/core/http/NetworkSettings.h>
#include <olp/core/porting/warning_disable.h>

PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace {

TEST(NetworkSettingsTest, TimeoutDefaults) {
  const auto settings = olp::http::NetworkSettings();
  EXPECT_EQ(settings.GetConnectionTimeout(), 60);
  EXPECT_EQ(settings.GetConnectionTimeoutDuration(), std::chrono::seconds(60));
  EXPECT_EQ(settings.GetTransferTimeout(), 30);
  EXPECT_EQ(settings.GetTransferTimeoutDuration(), std::chrono::seconds(30));
}

TEST(NetworkSettingsTest, WithConnectionTimeoutDeprecated) {
  const auto settings = olp::http::NetworkSettings().WithConnectionTimeout(15);
  EXPECT_EQ(settings.GetConnectionTimeout(), 15);
  EXPECT_EQ(settings.GetConnectionTimeoutDuration(), std::chrono::seconds(15));
}

TEST(NetworkSettingsTest, WithConnectionTimeout) {
  const auto settings = olp::http::NetworkSettings().WithConnectionTimeout(
      std::chrono::seconds(15));
  EXPECT_EQ(settings.GetConnectionTimeout(), 15);
  EXPECT_EQ(settings.GetConnectionTimeoutDuration(), std::chrono::seconds(15));
}

TEST(NetworkSettingsTest, WithTransferTimeoutDeprecated) {
  const auto settings = olp::http::NetworkSettings().WithTransferTimeout(15);
  EXPECT_EQ(settings.GetTransferTimeout(), 15);
  EXPECT_EQ(settings.GetTransferTimeoutDuration(), std::chrono::seconds(15));
}

TEST(NetworkSettingsTest, WithTransferTimeout) {
  const auto settings = olp::http::NetworkSettings().WithTransferTimeout(
      std::chrono::seconds(15));
  EXPECT_EQ(settings.GetTransferTimeout(), 15);
  EXPECT_EQ(settings.GetTransferTimeoutDuration(), std::chrono::seconds(15));
}

TEST(NetworkSettingsTest, WithRetriesDeprecated) {
  const auto settings = olp::http::NetworkSettings().WithRetries(5);
  EXPECT_EQ(settings.GetRetries(), 5);
}

}  // namespace

PORTING_POP_WARNINGS()
