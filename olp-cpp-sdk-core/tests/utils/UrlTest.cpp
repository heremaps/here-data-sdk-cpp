/*
 * Copyright (C) 2025 HERE Europe B.V.
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

#include <string>

#include <olp/core/utils/Url.h>

namespace {

using UrlTest = testing::Test;

TEST(UrlTest, ParseHostAndRest) {
  {
    SCOPED_TRACE("Bad Url");
    EXPECT_FALSE(olp::utils::Url::ParseHostAndRest("bad url").has_value());
  }

  {
    SCOPED_TRACE("Good Url");
    const auto result = olp::utils::Url::ParseHostAndRest(
        "https://account.api.here.com/oauth2/token");
    ASSERT_TRUE(result.has_value());
    const auto& host = result.value().first;
    const auto& rest = result.value().second;
    EXPECT_EQ(host, "https://account.api.here.com");
    EXPECT_EQ(rest, "/oauth2/token");
  }

  {
    SCOPED_TRACE("Good Url with port");
    const auto result = olp::utils::Url::ParseHostAndRest(
        "https://account.api.here.com:8080/oauth2/token");
    ASSERT_TRUE(result.has_value());
    const auto& host = result.value().first;
    const auto& rest = result.value().second;
    EXPECT_EQ(host, "https://account.api.here.com:8080");
    EXPECT_EQ(rest, "/oauth2/token");
  }
}
}  // namespace
