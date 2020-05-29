/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <cstdio>
#include <fstream>

#include <gtest/gtest.h>

namespace {
constexpr auto kTime = "Fri, 29 May 2020 11:07:45 GMT";
}  // namespace

TEST(AuthenticationClientTest, TimeParsing) {
  {
    SCOPED_TRACE("Parse time");

    std::tm tm = {};
    std::istringstream ss(kTime);
    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S %z");
#ifdef _WIN32
    auto t = _mkgmtime(&tm);
#else
    auto t = timegm(&tm);
#endif

    EXPECT_EQ(t, 1590750465);
  }
}
