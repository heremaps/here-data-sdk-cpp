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

#include "TimeUtils.h"

namespace olp {
namespace dataservice {
namespace write {

namespace {
constexpr auto kSecondsInMinute = 60;
constexpr auto kSecondsInHour = 3600;
constexpr auto kSecondsInDay = 86400;
int getSecondsToNextHour_internal(int tm_min, int tm_sec) {
  auto timeToNextHour = kSecondsInMinute - tm_sec;
  timeToNextHour += kSecondsInMinute * (59 - tm_min);
  return timeToNextHour;
}

int getSecondsToNextDay_internal(int tm_hour, int tm_min, int tm_sec) {
  auto timeToNextDay = getSecondsToNextHour_internal(tm_min, tm_sec);
  timeToNextDay += kSecondsInHour * (23 - tm_hour);
  return timeToNextDay;
}

int getSecondsToNextWeek_internal(int tm_wday, int tm_hour, int tm_min,
                                  int tm_sec) {
  auto timeToNextWeek = getSecondsToNextDay_internal(tm_hour, tm_min, tm_sec);
  timeToNextWeek += kSecondsInDay * (6 - tm_wday);
  return timeToNextWeek;
}
}  // namespace

std::chrono::seconds getSecondsToNextHour(int tm_min, int tm_sec) {
  return std::chrono::seconds{getSecondsToNextHour_internal(tm_min, tm_sec)};
}

std::chrono::seconds getSecondsToNextDay(int tm_hour, int tm_min, int tm_sec) {
  return std::chrono::seconds{
      getSecondsToNextDay_internal(tm_hour, tm_min, tm_sec)};
}

std::chrono::seconds getSecondsToNextWeek(int tm_wday, int tm_hour, int tm_min,
                                          int tm_sec) {
  return std::chrono::seconds{
      getSecondsToNextWeek_internal(tm_wday, tm_hour, tm_min, tm_sec)};
}

template <class Clock, class Duration>
std::chrono::milliseconds getDelayTillPeriod(
    FlushSettings::TimePeriod period,
    std::chrono::time_point<Clock, Duration> atime_point) {
  auto atime_t = Clock::to_time_t(atime_point);
  std::tm currentTimePoint;
  currentTimePoint = *localtime(&atime_t);

  if (period == FlushSettings::TimePeriod::Weekly) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        getSecondsToNextWeek(currentTimePoint.tm_wday, currentTimePoint.tm_hour,
                             currentTimePoint.tm_min, currentTimePoint.tm_sec));
  } else if (period == FlushSettings::TimePeriod::Daily) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        getSecondsToNextDay(currentTimePoint.tm_hour, currentTimePoint.tm_min,
                            currentTimePoint.tm_sec));
  } else {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        getSecondsToNextHour(currentTimePoint.tm_min, currentTimePoint.tm_sec));
  }
}

template std::chrono::milliseconds getDelayTillPeriod(
    FlushSettings::TimePeriod period,
    std::chrono::time_point<std::chrono::system_clock,
                            std::chrono::system_clock::duration>
        atime_point);
}  // namespace write
}  // namespace dataservice
}  // namespace olp
