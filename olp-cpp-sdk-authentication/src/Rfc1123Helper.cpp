/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

#include "Rfc1123Helper.h"

#include <cctype>
#include <limits>
#include <sstream>
#include <string>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "olp/core/logging/Log.h"

namespace olp {
namespace authentication {
namespace {

constexpr auto kLogTag = "Rfc1123Helper.cpp";
constexpr auto kGmtToken = "GMT";

using RfcMonthType = boost::gregorian::greg_month::value_type;
using DayType = boost::gregorian::greg_day::value_type;
using MonthType = boost::gregorian::greg_month::value_type;
using YearType = boost::gregorian::greg_year::value_type;
using HourType = boost::posix_time::time_duration::hour_type;
using MinuteType = boost::posix_time::time_duration::min_type;
using SecondType = boost::posix_time::time_duration::sec_type;

struct ParsedRfc1123DateTime {
  DayType day = 0;
  MonthType month = 0;
  YearType year = 0;
  HourType hour = 0;
  MinuteType minute = 0;
  SecondType second = 0;
};

static_assert(std::numeric_limits<DayType>::max() >= 31,
              "DayType must be able to store values up to 31");
static_assert(std::numeric_limits<MonthType>::max() >= 12,
              "MonthType must be able to store values up to 12");
static_assert(std::numeric_limits<YearType>::max() >= 9999,
              "YearType must be able to store values up to 9999");
static_assert(std::numeric_limits<HourType>::max() >= 23,
              "HourType must be able to store values up to 23");
static_assert(std::numeric_limits<MinuteType>::max() >= 59,
              "MinuteType must be able to store values up to 59");
static_assert(std::numeric_limits<SecondType>::max() >= 59,
              "SecondType must be able to store values up to 59");

std::string TrimDateHeaderValue(const std::string& value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return {};
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

bool ParseUnsignedInteger(const std::string& token, int* result) {
  if (token.empty()) {
    return false;
  }

  int value = 0;
  for (char c : token) {
    const auto ch = static_cast<unsigned char>(c);
    if (!std::isdigit(ch)) {
      return false;
    }

    const auto digit = static_cast<int>(ch - '0');
    if (value > (std::numeric_limits<int>::max() - digit) / 10) {
      return false;
    }
    value = value * 10 + digit;
  }

  *result = value;
  return true;
}

bool ParseMonthToken(const std::string& token, RfcMonthType* month) {
  if (token == "Jan") {
    *month = 1;
    return true;
  }
  if (token == "Feb") {
    *month = 2;
    return true;
  }
  if (token == "Mar") {
    *month = 3;
    return true;
  }
  if (token == "Apr") {
    *month = 4;
    return true;
  }
  if (token == "May") {
    *month = 5;
    return true;
  }
  if (token == "Jun") {
    *month = 6;
    return true;
  }
  if (token == "Jul") {
    *month = 7;
    return true;
  }
  if (token == "Aug") {
    *month = 8;
    return true;
  }
  if (token == "Sep") {
    *month = 9;
    return true;
  }
  if (token == "Oct") {
    *month = 10;
    return true;
  }
  if (token == "Nov") {
    *month = 11;
    return true;
  }
  if (token == "Dec") {
    *month = 12;
    return true;
  }
  return false;
}

bool ParseClockToken(const std::string& token, HourType* hour,
                     MinuteType* minute, SecondType* second) {
  if (token.size() != 8 || token[2] != ':' || token[5] != ':') {
    return false;
  }

  int parsed_hour = 0;
  int parsed_minute = 0;
  int parsed_second = 0;
  if (!ParseUnsignedInteger(token.substr(0, 2), &parsed_hour) ||
      !ParseUnsignedInteger(token.substr(3, 2), &parsed_minute) ||
      !ParseUnsignedInteger(token.substr(6, 2), &parsed_second)) {
    return false;
  }

  if (parsed_hour > 23 || parsed_minute > 59 || parsed_second > 59) {
    return false;
  }

  *hour = static_cast<HourType>(parsed_hour);
  *minute = static_cast<MinuteType>(parsed_minute);
  *second = static_cast<SecondType>(parsed_second);
  return true;
}

bool ParseWeekDay(std::istringstream& stream) {
  std::string weekday_token;
  return (stream >> weekday_token) && weekday_token.size() == 4 &&
         weekday_token.back() == ',';
}

bool ParseDay(std::istringstream& stream, ParsedRfc1123DateTime* parsed) {
  std::string day_token;
  int day = 0;
  if (!(stream >> day_token) || !ParseUnsignedInteger(day_token, &day) ||
      day == 0 || day > 31) {
    return false;
  }

  parsed->day = static_cast<DayType>(day);
  return true;
}

bool ParseMonth(std::istringstream& stream, ParsedRfc1123DateTime* parsed) {
  std::string month_token;
  MonthType month = 0;
  if (!(stream >> month_token) || !ParseMonthToken(month_token, &month)) {
    return false;
  }

  parsed->month = month;
  return true;
}

bool ParseYear(std::istringstream& stream, ParsedRfc1123DateTime* parsed) {
  std::string year_token;
  int year = 0;
  if (!(stream >> year_token) || !ParseUnsignedInteger(year_token, &year) ||
      year < 1400 || year > 9999) {
    return false;
  }

  parsed->year = static_cast<YearType>(year);
  return true;
}

bool ParseTimeOfDay(std::istringstream& stream, ParsedRfc1123DateTime* parsed) {
  std::string clock_token;
  return (stream >> clock_token) &&
         ParseClockToken(clock_token, &parsed->hour, &parsed->minute,
                         &parsed->second);
}

bool ParseTimeZone(std::istringstream& stream) {
  std::string timezone_token;
  return (stream >> timezone_token) && timezone_token == kGmtToken;
}

bool HasNoTrailingTokens(std::istringstream& stream) {
  std::string trailing_token;
  return !(stream >> trailing_token);
}

bool IsValidDateTime(const ParsedRfc1123DateTime& parsed) {
  const auto year_number = static_cast<unsigned short>(parsed.year);
  const auto month_number = static_cast<unsigned short>(parsed.month);
  const auto day_number = static_cast<unsigned short>(parsed.day);
  return day_number <= boost::gregorian::gregorian_calendar::end_of_month_day(
                           year_number, month_number);
}

}  // namespace

namespace internal {

std::time_t ParseRfc1123GmtNoExceptions(const std::string& value) {
  const auto trimmed_value = TrimDateHeaderValue(value);
  if (trimmed_value.empty()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "Failed to parse Date header '%s': value is empty "
                          "after trimming whitespace",
                          value.c_str());
    return static_cast<std::time_t>(-1);
  }

  std::istringstream stream(trimmed_value);
  ParsedRfc1123DateTime parsed;

  // Expected format: 'Thu, 1 Jan 1970 00:00:00 GMT'
  // Weekday & Timezone are ignored
  if (!ParseWeekDay(stream) ||             //
      !ParseDay(stream, &parsed) ||        //
      !ParseMonth(stream, &parsed) ||      //
      !ParseYear(stream, &parsed) ||       //
      !ParseTimeOfDay(stream, &parsed) ||  //
      !ParseTimeZone(stream)) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "Failed to parse Date header '%s': format mismatch "
                          "for RFC1123 timestamp",
                          value.c_str());
    return static_cast<std::time_t>(-1);
  }

  if (!HasNoTrailingTokens(stream)) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "Failed to parse Date header '%s': unexpected "
                          "trailing characters after timestamp",
                          value.c_str());
    return static_cast<std::time_t>(-1);
  }

  if (!IsValidDateTime(parsed)) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "Failed to parse Date header '%s': parsed value is "
                          "not a valid date/time",
                          value.c_str());
    return static_cast<std::time_t>(-1);
  }

  auto date = boost::gregorian::date(parsed.year, parsed.month, parsed.day);
  auto time = boost::posix_time::time_duration(parsed.hour, parsed.minute,
                                               parsed.second);
  const auto parsed_time = boost::posix_time::ptime(date, time);
  const auto epoch =
      boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1));

  if (parsed_time < epoch) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "Failed to parse Date header '%s': timestamp is before Unix epoch",
        value.c_str());
    return static_cast<std::time_t>(-1);
  }

  const auto seconds_since_epoch = (parsed_time - epoch).total_seconds();
  using T = boost::remove_cv_t<decltype(seconds_since_epoch)>;
  if (seconds_since_epoch >
      static_cast<T>(std::numeric_limits<std::time_t>::max())) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "Failed to parse Date header '%s': timestamp exceeds std::time_t range",
        value.c_str());
    return static_cast<std::time_t>(-1);
  }

  return static_cast<std::time_t>(seconds_since_epoch);
}

}  // namespace internal
}  // namespace authentication
}  // namespace olp
