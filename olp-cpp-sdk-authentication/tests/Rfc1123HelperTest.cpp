/*
 * Copyright (C) 2026 HERE Europe B.V.
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

#include "../src/Rfc1123Helper.h"

#include <ctime>

#include <gmock/gmock.h>

namespace {

struct ParseCase {
  const char* name;
  const char* input;
  std::time_t expected;
};

constexpr auto kParseFailed = static_cast<std::time_t>(-1);

class Rfc1123HelperParameterizedTest
    : public ::testing::TestWithParam<ParseCase> {};

TEST_P(Rfc1123HelperParameterizedTest, ParsesAsExpected) {
  const auto& test_case = GetParam();
  EXPECT_EQ(olp::authentication::internal::ParseRfc1123GmtNoExceptions(
                test_case.input),
            test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    ParsesAndRejectsRfc1123Dates, Rfc1123HelperParameterizedTest,
    ::testing::Values(
        // --- Valid inputs ---
        ParseCase{"UnixEpoch", "Thu, 01 Jan 1970 00:00:00 GMT",
                  static_cast<std::time_t>(0)},
        ParseCase{"SurroundingWhitespace",
                  "\t Tue, 13 Jan 2026 22:46:05 GMT \r\n",
                  static_cast<std::time_t>(1768344365)},
        ParseCase{"SingleDigitDay", "Thu, 1 Jan 1970 00:00:00 GMT",
                  static_cast<std::time_t>(0)},
        ParseCase{"MaxTimeOfDay", "Thu, 01 Jan 1970 23:59:59 GMT",
                  static_cast<std::time_t>(86399)},
        ParseCase{"RecentDate", "Sun, 22 Feb 2026 15:30:45 GMT",
                  static_cast<std::time_t>(1771774245)},
        ParseCase{"LeapYearFeb29", "Sat, 29 Feb 2020 12:00:00 GMT",
                  static_cast<std::time_t>(1582977600)},
        ParseCase{"EndOfYear", "Thu, 31 Dec 2020 23:59:59 GMT",
                  static_cast<std::time_t>(1609459199)},
        ParseCase{"March", "Sun, 8 Mar 2020 23:59:59 GMT",
                  static_cast<std::time_t>(1583711999)},
        ParseCase{"Birthday", "Wed, 8 Apr 2020 23:59:59 GMT",
                  static_cast<std::time_t>(1586390399)},
        ParseCase{"July", "Wed, 8 Jul 2020 23:59:59 GMT",
                  static_cast<std::time_t>(1594252799)},
        ParseCase{"August", "Sat, 8 Aug 2020 23:59:59 GMT",
                  static_cast<std::time_t>(1596931199)},
        // --- Before Epoch ---
        ParseCase{"MinValidYear", "Mon, 01 Jan 1400 00:00:00 GMT",
                  kParseFailed},
        // --- Empty / garbled input ---
        ParseCase{"EmptyString", "", kParseFailed},
        ParseCase{"WhitespaceOnly", "   \t\r\n  ", kParseFailed},
        ParseCase{"GarbledInput", "not a date", kParseFailed},
        ParseCase{"RandomNumbers", "12345", kParseFailed},
        // --- Invalid month ---
        ParseCase{"InvalidMonthToken", "Tue, 13 Foo 2026 22:46:05 GMT",
                  kParseFailed},
        // --- Invalid timezone ---
        ParseCase{"InvalidTimezoneToken", "Tue, 13 Jan 2026 22:46:05 UTC",
                  kParseFailed},
        ParseCase{"MissingTimezone", "Tue, 13 Jan 2026 22:46:05", kParseFailed},
        // --- Invalid day in month ---
        ParseCase{"InvalidDayInMonth", "Mon, 31 Feb 2025 10:11:12 GMT",
                  kParseFailed},
        ParseCase{"Feb29NonLeapYear", "Sat, 29 Feb 2025 12:00:00 GMT",
                  kParseFailed},
        ParseCase{"DayZero", "Thu, 0 Jan 1970 00:00:00 GMT", kParseFailed},
        ParseCase{"DayTooLarge", "Thu, 32 Jan 1970 00:00:00 GMT", kParseFailed},
        // --- Invalid clock values ---
        ParseCase{"HourTooLarge", "Thu, 01 Jan 1970 25:00:00 GMT",
                  kParseFailed},
        ParseCase{"MinuteTooLarge", "Thu, 01 Jan 1970 12:60:00 GMT",
                  kParseFailed},
        ParseCase{"SecondTooLarge", "Thu, 01 Jan 1970 12:00:61 GMT",
                  kParseFailed},
        ParseCase{"MalformedClock", "Thu, 01 Jan 1970 1:2:3 GMT", kParseFailed},
        // --- Invalid year ---
        ParseCase{"YearBelowMinimum", "Mon, 01 Jan 1399 00:00:00 GMT",
                  kParseFailed},
        ParseCase{"YearAboveMaximum", "Mon, 01 Jan 10000 00:00:00 GMT",
                  kParseFailed},
        // --- Pre-epoch ---
        ParseCase{"PreEpoch", "Wed, 31 Dec 1969 23:59:59 GMT", kParseFailed},
        // --- Trailing garbage ---
        ParseCase{"TrailingGarbage",
                  "Thu, 01 Jan 1970 00:00:00 GMT extra stuff", kParseFailed},
        ParseCase{"TrailingSingleToken", "Thu, 01 Jan 1970 00:00:00 GMT X",
                  kParseFailed},
        // --- Structural issues ---
        ParseCase{"MissingCommaAfterWeekday", "Thu 01 Jan 1970 00:00:00 GMT",
                  kParseFailed},
        ParseCase{"WeekdayTooShort", "Th, 01 Jan 1970 00:00:00 GMT",
                  kParseFailed},
        ParseCase{"WeekdayTooLong", "Thurs, 01 Jan 1970 00:00:00 GMT",
                  kParseFailed},
        // --- Specific edge cases ---
        ParseCase{"DayAsLetters", "Thu, AA Dec 2020 23:59:59 GMT",
                  static_cast<std::time_t>(kParseFailed)},
        ParseCase{"NonNumericClock", "Thu, 12 Dec 2020 11:AA:10 GMT",
                  static_cast<std::time_t>(kParseFailed)}

        ),
    [](const ::testing::TestParamInfo<ParseCase>& info) {
      return info.param.name;
    });

}  // namespace
