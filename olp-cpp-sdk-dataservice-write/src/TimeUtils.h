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

#pragma once

#include <olp/dataservice/write/FlushSettings.h>
#include <time.h>
#include <chrono>
#include <ctime>

namespace olp {
namespace dataservice {
namespace write {
std::chrono::seconds getSecondsToNextHour(int tm_min, int tm_sec);
std::chrono::seconds getSecondsToNextDay(int tm_hour, int tm_min, int tm_sec);
std::chrono::seconds getSecondsToNextWeek(int tm_wday, int tm_hour, int tm_min,
                                          int tm_sec);

template <class Clock, class Duration = typename Clock::duration>
std::chrono::milliseconds getDelayTillPeriod(
    FlushSettings::TimePeriod period,
    std::chrono::time_point<Clock, Duration> atime_point =
        std::chrono::system_clock::now());

}  // namespace write
}  // namespace dataservice
}  // namespace olp