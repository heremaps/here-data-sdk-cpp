/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <olp/core/client/OlpClientSettings.h>

namespace {

TEST(BackdownStrategy, Assign) {
  {
    olp::client::OlpClientSettings settings;
    settings.retry_settings.backdown_strategy =
        olp::client::EqualJitterBackdownStrategy(std::chrono::seconds(5));
  }
  {
    olp::client::OlpClientSettings settings;
    settings.retry_settings.backdown_strategy =
        olp::client::ExponentialBackdownStrategy();
  }
}

TEST(BackdownStrategy, EqualJitter_Cap) {
  const auto cap = std::chrono::seconds(5);
  olp::client::EqualJitterBackdownStrategy backdown_strategy(cap);
  const auto base = std::chrono::milliseconds(200);
  for (auto retry = 0; retry < 100; ++retry) {
    const auto wait_time = backdown_strategy(base, retry);
    EXPECT_LE(wait_time, cap);
  }
}

}  // namespace
