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

#include <chrono>
#include <ctime>

#include <gtest/gtest.h>
#include "TimeUtils.h"

namespace {

using namespace olp::dataservice::write;

template <typename Clock = std::chrono::system_clock>
auto ToTimePoint(std::tm a_tm) -> typename Clock::time_point {
  auto time_c = mktime(&a_tm);
  return Clock::from_time_t(time_c);
}

bool IsDst(std::tm timePoint) {
  auto time_c = mktime(&timePoint);
  std::tm a_tm;
  a_tm = *localtime(&time_c);
  return a_tm.tm_isdst != 0;
}

TEST(TimeUtilsTest, Subroutine) {
  ASSERT_EQ(std::chrono::seconds(3600), getSecondsToNextHour(0, 0));
  ASSERT_EQ(std::chrono::seconds(3599), getSecondsToNextHour(0, 1));
  ASSERT_EQ(std::chrono::seconds(1), getSecondsToNextHour(59, 59));
  ASSERT_EQ(std::chrono::seconds(0), getSecondsToNextHour(59, 60));

  ASSERT_EQ(std::chrono::seconds(86400), getSecondsToNextDay(0, 0, 0));
  ASSERT_EQ(std::chrono::seconds(86399), getSecondsToNextDay(0, 0, 1));
  ASSERT_EQ(std::chrono::seconds(82800), getSecondsToNextDay(1, 0, 0));
  ASSERT_EQ(std::chrono::seconds(1), getSecondsToNextDay(23, 59, 59));
  ASSERT_EQ(std::chrono::seconds(0), getSecondsToNextDay(23, 59, 60));

  ASSERT_EQ(std::chrono::seconds(604800), getSecondsToNextWeek(0, 0, 0, 0));
  ASSERT_EQ(std::chrono::seconds(604799), getSecondsToNextWeek(0, 0, 0, 1));
  ASSERT_EQ(std::chrono::seconds(604740), getSecondsToNextWeek(0, 0, 1, 0));
  ASSERT_EQ(std::chrono::seconds(601200), getSecondsToNextWeek(0, 1, 0, 0));
  ASSERT_EQ(std::chrono::seconds(518400), getSecondsToNextWeek(1, 0, 0, 0));
  ASSERT_EQ(std::chrono::seconds(518399), getSecondsToNextWeek(1, 0, 0, 1));
  ASSERT_EQ(std::chrono::seconds(1), getSecondsToNextWeek(6, 23, 59, 59));
  ASSERT_EQ(std::chrono::seconds(0), getSecondsToNextWeek(6, 23, 59, 60));
}

TEST(TimeUtilsTest, Period) {
  std::tm testTimePoint = {0};
  testTimePoint.tm_year = 2018 - 1900;  // [0, 60] since 1900
  testTimePoint.tm_mon = 6 - 1;         // [0, 11] since Jan
  testTimePoint.tm_mday = 10;           // [1, 31]
  testTimePoint.tm_hour = 6;            // [0, 23] since midnight
  testTimePoint.tm_min = 30;            // [0, 59] after the hour
  testTimePoint.tm_sec =
      30;  // [0, 60] after the min allows for 1 positive leap second
  testTimePoint.tm_isdst =
      0;  // [-1...] -1 for unknown, 0 for not DST, any positive value if DST.

  ASSERT_EQ(std::chrono::milliseconds(1770000ull),
            getDelayTillPeriod(FlushSettings::TimePeriod::Hourly,
                               ToTimePoint(testTimePoint)));
  if (IsDst(testTimePoint)) {
    ASSERT_EQ(std::chrono::milliseconds(59370000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Daily,
                                 ToTimePoint(testTimePoint)));
    ASSERT_EQ(std::chrono::milliseconds(577770000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Weekly,
                                 ToTimePoint(testTimePoint)));
  } else {
    ASSERT_EQ(std::chrono::milliseconds(62970000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Daily,
                                 ToTimePoint(testTimePoint)));
    ASSERT_EQ(std::chrono::milliseconds(581370000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Weekly,
                                 ToTimePoint(testTimePoint)));
  }

  testTimePoint = std::tm{};
  testTimePoint.tm_year = 2018 - 1900;
  testTimePoint.tm_mon = 5;
  testTimePoint.tm_mday = 29;
  testTimePoint.tm_hour = 23;
  testTimePoint.tm_min = 58;
  testTimePoint.tm_sec = 30;
  testTimePoint.tm_isdst = 0;

  ASSERT_EQ(std::chrono::milliseconds(90000ull),
            getDelayTillPeriod(FlushSettings::TimePeriod::Hourly,
                               ToTimePoint(testTimePoint)));
  if (IsDst(testTimePoint)) {
    ASSERT_EQ(std::chrono::milliseconds(82890000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Daily,
                                 ToTimePoint(testTimePoint)));
    ASSERT_EQ(std::chrono::milliseconds(82890000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Weekly,
                                 ToTimePoint(testTimePoint)));
  } else {
    ASSERT_EQ(std::chrono::milliseconds(90000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Daily,
                                 ToTimePoint(testTimePoint)));
    ASSERT_EQ(std::chrono::milliseconds(86490000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Weekly,
                                 ToTimePoint(testTimePoint)));
  }

  testTimePoint = std::tm{};
  testTimePoint.tm_year = 2018 - 1900;
  testTimePoint.tm_mon = 5;
  testTimePoint.tm_mday = 29;
  testTimePoint.tm_hour = 22;
  testTimePoint.tm_min = 58;
  testTimePoint.tm_sec = 30;
  testTimePoint.tm_isdst = 0;

  ASSERT_EQ(std::chrono::milliseconds(90000ull),
            getDelayTillPeriod(FlushSettings::TimePeriod::Hourly,
                               ToTimePoint(testTimePoint)));
  if (IsDst(testTimePoint)) {
    ASSERT_EQ(std::chrono::milliseconds(90000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Daily,
                                 ToTimePoint(testTimePoint)));
    ASSERT_EQ(std::chrono::milliseconds(86490000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Weekly,
                                 ToTimePoint(testTimePoint)));
  } else {
    ASSERT_EQ(std::chrono::milliseconds(3690000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Daily,
                                 ToTimePoint(testTimePoint)));
    ASSERT_EQ(std::chrono::milliseconds(90090000ull),
              getDelayTillPeriod(FlushSettings::TimePeriod::Weekly,
                                 ToTimePoint(testTimePoint)));
  }
}

}  // namespace
