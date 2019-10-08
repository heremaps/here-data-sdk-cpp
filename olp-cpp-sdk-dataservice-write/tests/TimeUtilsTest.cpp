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

}  // namespace
