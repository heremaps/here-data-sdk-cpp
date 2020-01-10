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

#include <string>

#include <gtest/gtest.h>
#include <olp/core/logging/Format.h>
#include <olp/core/porting/platform.h>

namespace {

using namespace olp::logging;
using namespace testing;

TEST(FormatTest, FormatString) {
  ASSERT_EQ(std::string(), format("%s", ""));

  std::string string = format("% 4d %s %s %f", 6, "foo", "bar", 2.45f);
  EXPECT_EQ("   6 foo bar 2.450000", string);
}

TEST(FormatTest, FormatStringOneLessThanBuffer) {
  const unsigned int buffer_size = 255;
  char base_string[buffer_size + 1];
  for (unsigned int i = 0; i < buffer_size; ++i) base_string[i] = ' ';
  base_string[buffer_size] = 0;
  std::string string = format("%s", base_string);
  EXPECT_EQ(base_string, string);
}

TEST(FormatTest, FormatStringEqualsBuffer) {
  const unsigned int buffer_size = 256;
  char base_string[buffer_size + 1];
  for (unsigned int i = 0; i < buffer_size; ++i) base_string[i] = ' ';
  base_string[buffer_size] = 0;
  std::string string = format("%s", base_string);
  EXPECT_EQ(base_string, string);
}

TEST(FormatTest, FormatStringOneMoreThanBuffer) {
  const unsigned int buffer_size = 257;
  char base_string[buffer_size + 1];
  for (unsigned int i = 0; i < buffer_size; ++i) base_string[i] = ' ';
  base_string[buffer_size] = 0;
  std::string string = format("%s", base_string);
  EXPECT_EQ(base_string, string);
}

TEST(FormatTest, FormatStringLarge) {
  const char* base_string = "This is a very very very very large string.";
  std::string compare;
  for (unsigned int i = 0; i < 10; ++i) compare += base_string;

  std::string string = format("%s%s%s%s%s%s%s%s%s%s", base_string, base_string,
                              base_string, base_string, base_string, base_string,
                              base_string, base_string, base_string, base_string);

  EXPECT_EQ(compare, string);
  EXPECT_LT(256U, string.size());
}

TEST(FormatTest, FormatLocalTime) {
  TimePoint test_time = std::chrono::system_clock::from_time_t(time_t(1234));
  EXPECT_FALSE(formatLocalTime(test_time).empty());
}

TEST(FormatTest, FormatLocalTimeFormatStr) {
  TimePoint test_time = std::chrono::system_clock::from_time_t(time_t(1234));
  EXPECT_FALSE(formatLocalTime(test_time, "%H:%M:%S").empty());

#ifndef PORTING_PLATFORM_WINDOWS
  // %q is an invalid format string. Windows aborts under debug for improper
  // format strings.
  EXPECT_FALSE(formatLocalTime(test_time, "%q").empty());
#endif
}

TEST(FormatTest, FormatLocalTimeLarge) {
  TimePoint test_time = std::chrono::system_clock::from_time_t(time_t(1234));
  const char* base_string = "This is a very very very very large string.";
  std::string format_string;
  for (unsigned int i = 0; i < 20; ++i) format_string += base_string;
  EXPECT_EQ(format_string, formatLocalTime(test_time, format_string.c_str()));
}

TEST(FormatTest, FormatUtcTime) {
  TimePoint test_time = std::chrono::system_clock::from_time_t(time_t(1234));
  EXPECT_FALSE(formatUtcTime(test_time).empty());
}

TEST(FormatTest, FormatUtcTimeFormatStr) {
  TimePoint test_time = std::chrono::system_clock::from_time_t(time_t(1234));
  EXPECT_FALSE(formatUtcTime(test_time, "%H:%M:%S").empty());

#ifndef PORTING_PLATFORM_WINDOWS
  // %q is an invalid format string. Windows aborts under debug for improper
  // format strings.
  EXPECT_FALSE(formatUtcTime(test_time, "%q").empty());
#endif
}

TEST(FormatTest, FormatUtcTimeLarge) {
  TimePoint test_time = std::chrono::system_clock::from_time_t(time_t(1234));
  const char* base_string = "This is a very very very very large string.";
  std::string format_string;
  for (unsigned int i = 0; i < 20; ++i) format_string += base_string;
  EXPECT_EQ(format_string, formatUtcTime(test_time, format_string.c_str()));
}

TEST(FormatTest, FormatBuffer) {
  FormatBuffer buffer;
  ASSERT_STREQ("", buffer.format("%s", ""));

  const char* string = buffer.format("% 4d %s %s %f", 6, "foo", "bar", 2.45f);
  EXPECT_STREQ("   6 foo bar 2.450000", string);
}

TEST(FormatTest, FormatBufferLarge) {
  const char* base_string = "This is a very very very very large string.";
  std::string compare;
  for (unsigned int i = 0; i < 10; ++i) compare += base_string;

  std::string string = format("%s%s%s%s%s%s%s%s%s%s", base_string, base_string,
                              base_string, base_string, base_string, base_string,
                              base_string, base_string, base_string, base_string);

  EXPECT_EQ(compare, string);
  EXPECT_LT(256U, compare.size());
}

}  // namespace
