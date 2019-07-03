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

#include <olp/core/logging/Format.h>
#include <olp/core/porting/platform.h>
#include <stdio.h>
#include <time.h>
#include <cassert>
#include <string>

#ifdef PORTING_PLATFORM_WINDOWS
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define swprintf _snwprintf
#define vswprintf _vsnwprintf
#endif

namespace olp {
namespace logging {
static const char* defaultTimeFormatStr = "%Y-%m-%d %H:%M:%S";

static int snprintfFixed(char* buffer, std::size_t size, const char* formatStr,
                         va_list args) {
#ifdef PORTING_PLATFORM_WINDOWS
  // Under windows, vsnprintf() returns -1 if the buffer isn't large enough.
  int ret = vsnprintf(buffer, size, formatStr, args);
  if (ret < 0) {
    // Need to call a separate function to get the number of characters if
    // snprintf failed
    ret = _vscprintf(formatStr, args);
  }
  return ret;
#else
  // Other platforms behave properly.
  va_list argsCopy;
  va_copy(argsCopy, args);
  int ret = vsnprintf(buffer, size, formatStr, argsCopy);
  va_end(argsCopy);
  return ret;
#endif
}

static time_t toTimeT(const TimePoint& timestamp) {
  // NOTE: It is implementation defined if
  // std::chrono::system_clock::to_time_t() rounds or truncates. Need to
  // truncate if we want to also print out milliseconds. (duration_cast() will
  // always truncate)
  TimePoint truncatedTimestamp(std::chrono::duration_cast<std::chrono::seconds>(
      timestamp.time_since_epoch()));
  return std::chrono::system_clock::to_time_t(truncatedTimestamp);
}

std::string format(const char* const formatStr, ...) {
  va_list args;
  va_start(args, formatStr);
  std::string ret = formatv(formatStr, args);
  va_end(args);
  return ret;
}

std::string formatv(const char* formatStr, va_list args) {
  FormatBuffer formatBuffer;
  return formatBuffer.formatv(formatStr, args);
}

std::string formatLocalTime(const TimePoint& timestamp) {
  FormatBuffer formatBuffer;
  return formatBuffer.formatLocalTime(timestamp);
}

std::string formatLocalTime(const TimePoint& timestamp, const char* formatStr) {
  FormatBuffer formatBuffer;
  return formatBuffer.formatLocalTime(timestamp, formatStr);
}

std::string formatUtcTime(const TimePoint& timestamp) {
  FormatBuffer formatBuffer;
  return formatBuffer.formatUtcTime(timestamp);
}

std::string formatUtcTime(const TimePoint& timestamp, const char* formatStr) {
  FormatBuffer formatBuffer;
  return formatBuffer.formatUtcTime(timestamp, formatStr);
}

const char* FormatBuffer::format(const char* formatStr, ...) {
  va_list args;
  va_start(args, formatStr);
  const char* retVal = formatv(formatStr, args);
  va_end(args);
  return retVal;
}

const char* FormatBuffer::formatv(const char* formatStr, va_list args) {
  m_buffer[0] = 0;
  if (!formatStr) return m_buffer;

  // First try the local buffer.
  const int numCharsWritten =
      snprintfFixed(m_buffer, bufferSize, formatStr, args);

  if (numCharsWritten < 0)
    return m_buffer;
  else if (size_t(numCharsWritten) >= bufferSize) {
    // Use a vector for the call to snprintf because std::string isn't
    // guaranteed to be contiguous in memory.
    const size_t numChars = static_cast<size_t>(numCharsWritten + 1);
    m_auxBuffer.resize(numChars);

    snprintfFixed(m_auxBuffer.data(), numChars, formatStr, args);
    return m_auxBuffer.data();
  } else
    return m_buffer;
}

const char* FormatBuffer::formatLocalTime(const TimePoint& timestamp) {
  return formatLocalTime(timestamp, defaultTimeFormatStr);
}

const char* FormatBuffer::formatLocalTime(const TimePoint& timestamp,
                                          const char* formatStr) {
  time_t timestampTime = toTimeT(timestamp);
  struct tm timestampTm;
#ifdef PORTING_PLATFORM_WINDOWS
  localtime_s(&timestampTm, &timestampTime);
#else
  localtime_r(&timestampTime, &timestampTm);
#endif

  return formatTm(timestampTm, formatStr);
}

const char* FormatBuffer::formatUtcTime(const TimePoint& timestamp) {
  return formatUtcTime(timestamp, defaultTimeFormatStr);
}

const char* FormatBuffer::formatUtcTime(const TimePoint& timestamp,
                                        const char* formatStr) {
  time_t timestampTime = toTimeT(timestamp);
  struct tm timestampTm;
#ifdef PORTING_PLATFORM_WINDOWS
  gmtime_s(&timestampTm, &timestampTime);
#else
  gmtime_r(&timestampTime, &timestampTm);
#endif

  return formatTm(timestampTm, formatStr);
}

const char* FormatBuffer::formatTm(const struct tm& timestampTm,
                                   const char* formatStr) {
  // First try the local buffer.
  if (strftime(m_buffer, bufferSize, formatStr, &timestampTm)) return m_buffer;

  // Fall back to the auxiliary buffer if not large enough.
  m_auxBuffer.resize(bufferSize * 2);
  for (;;) {
    if (strftime(m_auxBuffer.data(), m_auxBuffer.size(), formatStr,
                 &timestampTm))
      return m_auxBuffer.data();

    // Double the buffer and try again.
    m_auxBuffer.resize(m_auxBuffer.size() * 2, 0);
  }
}

}  // namespace logging
}  // namespace olp
