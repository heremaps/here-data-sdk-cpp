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

#include <olp/core/CoreApi.h>

#include <chrono>
#include <cstdarg>
#include <string>
#include <vector>

#if (defined(__GNUC__) || defined(__clang__))
#define CHECK_PRINTF_FORMAT_STRING(format_string_pos, first_to_check) \
  __attribute__((format(printf, (format_string_pos), (first_to_check))))
#else
#define CHECK_PRINTF_FORMAT_STRING(format_string_pos, first_to_check)
#endif

namespace olp {
namespace logging {
/**
 * @brief A typedef for a time point from the system clock.
 */
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

/**
 * @brief Formats a string using a printf-style format string.
 *
 * @param formatStr The format string.
 *
 * @return The formatted string.
 */
CORE_API std::string format(const char* formatStr, ...)
    CHECK_PRINTF_FORMAT_STRING(1, 2);

/**
 * @brief Formats a string using a printf-style format string with an already
 * created `va_list` object.
 *
 * @param formatStr The format string.
 * @param args The `va_list` object that contains the arguments.
 *
 * @return The formatted string.
 */
CORE_API std::string formatv(const char* formatStr, va_list args);

/**
 * @brief Creates a string for a timestamp using local time with the default
 * format string.
 *
 * The default format string is `%Y-%m-%d %H:%M:%S`.
 *
 * @param timestamp The timestamp to format as seconds from epoch.
 *
 * @return The formatted string.
 */
CORE_API std::string formatLocalTime(const TimePoint& timestamp);

/**
 * @brief Creates a string for a timestamp using local time.
 *
 * @param timestamp The timestamp to format as seconds from epoch.
 * @param formatStr The format string to use, conforming to `strftime`.
 *
 * @return The formatted string.
 */
CORE_API std::string formatLocalTime(const TimePoint& timestamp,
                                    const char* formatStr);

/**
 * @brief Creates a string for a timestamp using the UTC time standard with the default
 * format string.
 *
 * The default format string is `%Y-%m-%d %H:%M:%S`.
 *
 * @param timestamp The timestamp to format as seconds from epoch.
 *
 * @return The formatted string.
 */
CORE_API std::string formatUtcTime(const TimePoint& timestamp);

/**
 * @brief Creates a string for a timestamp using the UTC time standard.
 *
 * @param timestamp The timestamp to format as seconds from epoch.
 * @param formatStr The format string to use, conforming to `strftime`.
 *
 * @return The formatted string.
 */
CORE_API std::string formatUtcTime(const TimePoint& timestamp,
                                  const char* formatStr);

/**
 * @brief Attempts to format a string to a buffer before falling back
 * to a dynamically allocated string.
 *
 * This can be used to avoid allocating `std::string` for smaller strings.
 */
class CORE_API FormatBuffer {
 public:
  /**
   * @brief Formats a string using a printf-style format string.
   *
   * @param formatStr The format string.
   *
   * @return The formatted string.
   */
  const char* format(const char* formatStr, ...)
      CHECK_PRINTF_FORMAT_STRING(2, 3);

  /**
   * @brief Formats a string using a printf-style format string with an already
   * created `va_list`.
   *
   * @param formatStr The format string.
   * @param args The va_list that contains the arguments.
   *
   * @return The formatted string.
   */
  const char* formatv(const char* formatStr, va_list args);

  /**
   * @brief Creates a string for a timestamp using local time with the default
   * format string.
   *
   * The default format string is `%Y-%m-%d %H:%M:%S`.
   *
   * @param timestamp The timestamp to format as seconds from epoch.
   *
   * @return The formatted string.
   */
  const char* formatLocalTime(const TimePoint& timestamp);

  /**
   * @brief Creates a string for a timestamp using local time.
   *
   * @param timestamp The timestamp to format as seconds from epoch.
   * @param formatStr The format string to use, conforming to strftime.
   *
   * @return The formatted string.
   */
  const char* formatLocalTime(const TimePoint& timestamp,
                              const char* formatStr);

  /**
   * @brief Creates a string for a timestamp using the UTC time standard with the default
   * format string.
   *
   * The default format string is `%Y-%m-%d %H:%M:%S`.
   *
   * @param timestamp The timestamp to format as seconds from epoch.
   *
   * @return The formatted string.
   */
  const char* formatUtcTime(const TimePoint& timestamp);

  /**
   * @brief Creates a string for a timestamp using the UTC time standard.
   *
   * @param timestamp The timestamp to format as seconds from epoch.
   * @param formatStr The format string to use, conforming to strftime.
   *
   * @return The formatted string.
   */
  const char* formatUtcTime(const TimePoint& timestamp, const char* formatStr);

 private:
  friend std::string logging::formatv(const char* formatStr, va_list args);

  const char* formatTm(const struct tm& timestampTm, const char* formatStr);

  static const unsigned int bufferSize = 256;
  char m_buffer[bufferSize];
  std::vector<char> m_auxBuffer;
};

}  // namespace logging
}  // namespace olp
