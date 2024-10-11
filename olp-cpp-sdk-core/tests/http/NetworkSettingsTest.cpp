/*
 * Copyright (C) 2023-2024 HERE Europe B.V.
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

namespace {

TEST(NetworkSettingsTest, TimeoutDefaults) {
  const auto settings = olp::http::NetworkSettings();
  EXPECT_EQ(settings.GetConnectionTimeoutDuration(), std::chrono::seconds(60));
  EXPECT_EQ(settings.GetTransferTimeoutDuration(), std::chrono::seconds(30));
}

TEST(NetworkSettingsTest, WithConnectionTimeout) {
  const auto settings = olp::http::NetworkSettings().WithConnectionTimeout(
      std::chrono::seconds(15));
  EXPECT_EQ(settings.GetConnectionTimeoutDuration(), std::chrono::seconds(15));
}

TEST(NetworkSettingsTest, WithTransferTimeout) {
  const auto settings = olp::http::NetworkSettings().WithTransferTimeout(
      std::chrono::seconds(15));
  EXPECT_EQ(settings.GetTransferTimeoutDuration(), std::chrono::seconds(15));
}

TEST(NetworkSettingsTest, WithMaxConnectionLifetimeDefault) {
  const auto settings = olp::http::NetworkSettings();
  EXPECT_EQ(settings.GetMaxConnectionLifetime(), std::chrono::seconds(0));
}

TEST(NetworkSettingsTest, WithMaxConnectionLifetime) {
  const auto settings = olp::http::NetworkSettings().WithMaxConnectionLifetime(
      std::chrono::seconds(15));
  EXPECT_EQ(settings.GetMaxConnectionLifetime(), std::chrono::seconds(15));
}

TEST(NetworkSettingsTest, WithDNSServers) {
  const std::vector<std::string> expected = {"1.1.1.1", "1.1.1.2"};
  const auto settings = olp::http::NetworkSettings().WithDNSServers(expected);
  EXPECT_EQ(settings.GetDNSServers(), expected);
}

}  // namespace
