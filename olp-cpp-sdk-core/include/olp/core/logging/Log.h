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

#include <olp/core/logging/Format.h>
#include <olp/core/logging/Level.h>

#include <olp/core/CoreApi.h>
#include <olp/core/utils/WarningWorkarounds.h>

#include <boost/none.hpp>
#include <boost/optional.hpp>

#include <cinttypes>
#include <cstdlib>
#include <sstream>
#include <string>

/**
 * @file
 * @brief The file that provides the main interface to the logging library.
 */

/**
 * @brief Macro to get the current's function signature for different compilers
 */
#if __GNUC__ >= 3 || defined(__clang__)
#define LOG_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define LOG_FUNCTION_SIGNATURE __FUNCSIG__
#else
#define LOG_FUNCTION_SIGNATURE __FUNCTION__
#endif

/**
 * @brief While statement for "do {} while (0)" used for macros that bypasses
 * compiler warnings.
 */
#ifdef _MSC_VER
#define CORE_LOOP_ONCE()                                                  \
  __pragma(warning(push)) __pragma(warning(disable : 4127)) while (false) \
      __pragma(warning(pop))
#else
#define CORE_LOOP_ONCE() while (false)
#endif

/**
 * @brief Log a message using C++ style streams.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param level Logging level.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define DO_LOG(level, tag, message)                                     \
  do {                                                                  \
    std::stringstream __strm;                                           \
    __strm << message;                                                  \
    ::olp::logging::Log::logMessage(level, tag, __strm.str(), __FILE__, \
                                    __LINE__, __FUNCTION__,             \
                                    LOG_FUNCTION_SIGNATURE);            \
  }                                                                     \
  CORE_LOOP_ONCE()

/**
 * @brief Log a critical message using C++ style streams.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param level Logging level.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_CRITICAL(level, tag, message) DO_LOG(level, tag, message)

/**
 * @brief Log a critical info message using C++ style streams.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_CRITICAL_INFO(tag, message) \
  LOG_CRITICAL(::olp::logging::Level::Info, tag, message)

/**
 * @brief Log a critical warning message using C++ style streams.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_CRITICAL_WARNING(tag, message) \
  LOG_CRITICAL(::olp::logging::Level::Warning, tag, message)

/**
 * @brief Log a critical error message using C++ style streams.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_CRITICAL_ERROR(tag, message) \
  LOG_CRITICAL(::olp::logging::Level::Error, tag, message)

/**
 * @brief Log a fatal error message using C++ style streams.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_FATAL(tag, message) \
  LOG_CRITICAL(::olp::logging::Level::Fatal, tag, message)

/**
 * @brief Log a critical fatal error message using C++ style streams, then abort
 * the program.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_ABORT(tag, message) \
  do {                          \
    LOG_FATAL(tag, message);    \
    std::abort();               \
  }                             \
  CORE_LOOP_ONCE()

/**
 * @brief Log a message using printf style formatting.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param level Logging level.
 * @param tag Log component name.
 */
#define DO_LOG_F(level, tag, ...)                                              \
  do {                                                                         \
    std::string __message = ::olp::logging::format(__VA_ARGS__);               \
    ::olp::logging::Log::logMessage(level, tag, __message, __FILE__, __LINE__, \
                                    __FUNCTION__, LOG_FUNCTION_SIGNATURE);     \
  }                                                                            \
  CORE_LOOP_ONCE()

/**
 * @brief Log a critical message using printf style formatting.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param level Logging level.
 * @param tag Log component name.
 */
#define LOG_CRITICAL_F(level, tag, ...) DO_LOG_F(level, tag, __VA_ARGS__)

/**
 * @brief Log a critical info message using printf style formatting.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 */
#define LOG_CRITICAL_INFO_F(tag, ...) \
  LOG_CRITICAL_F(::olp::logging::Level::Info, tag, __VA_ARGS__)

/**
 * @brief Log a critical warning message using printf style formatting.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 */
#define LOG_CRITICAL_WARNING_F(tag, ...) \
  LOG_CRITICAL_F(::olp::logging::Level::Warning, tag, __VA_ARGS__)

/**
 * @brief Log a critical error message using printf style formatting.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 */
#define LOG_CRITICAL_ERROR_F(tag, ...) \
  LOG_CRITICAL_F(::olp::logging::Level::Error, tag, __VA_ARGS__)

/**
 * @brief Log a critical fatal error message using printf style formatting.
 *
 * LOGGING_DISABLED does not disable this functionality. Additionally, this will
 * not check to see if the tag is disabled.
 *
 * @param tag Log component name.
 */
#define LOG_FATAL_F(tag, ...) \
  LOG_CRITICAL_F(::olp::logging::Level::Fatal, tag, __VA_ARGS__)

/**
 * @brief Log a critical fatal error message using printf style formatting, then
 * abort the program.
 * @param tag Log component name.
 */
#define LOG_ABORT_F(tag, ...)      \
  do {                             \
    LOG_FATAL_F(tag, __VA_ARGS__); \
    std::abort();                  \
  }                                \
  CORE_LOOP_ONCE()

#ifdef LOGGING_DISABLED
#define LOG(level, tag, message) \
  do {                           \
  }                              \
  CORE_LOOP_ONCE()
#else

/**
 * @brief Log a message using C++ style streams.
 * @param level Logging level.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG(level, tag, message)                      \
  do {                                                \
    if (::olp::logging::Log::isEnabled(level, tag)) { \
      DO_LOG(level, tag, message);                    \
    }                                                 \
  }                                                   \
  CORE_LOOP_ONCE()

#endif  // LOGGING_DISABLED

#ifdef LOGGING_DISABLE_DEBUG_LEVEL
#define LOG_TRACE(tag, message)           \
  do {                                    \
    ::olp::logging::NullLogStream __strm; \
    __strm << message;                    \
    CORE_UNUSED(tag, __strm);             \
  }                                       \
  CORE_LOOP_ONCE()

#define LOG_DEBUG(tag, message)           \
  do {                                    \
    ::olp::logging::NullLogStream __strm; \
    __strm << message;                    \
    CORE_UNUSED(tag, __strm);             \
  }                                       \
  CORE_LOOP_ONCE()

#else
/**
 * @brief Log a trace message using C++ style streams.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_TRACE(tag, message) LOG(::olp::logging::Level::Trace, tag, message)

/**
 * @brief Log a debug message using C++ style streams.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_DEBUG(tag, message) LOG(::olp::logging::Level::Debug, tag, message)

#endif  // LOGGING_DISABLE_DEBUG_LEVEL

/**
 * @brief Log an info message using C++ style streams.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_INFO(tag, message) LOG(::olp::logging::Level::Info, tag, message)

/**
 * @brief Log a warning message using C++ style streams.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_WARNING(tag, message) \
  LOG(::olp::logging::Level::Warning, tag, message)

/**
 * @brief Log an error message using C++ style streams.
 * @param tag Log component name.
 * @param message The message to log.
 */
#define LOG_ERROR(tag, message) LOG(::olp::logging::Level::Error, tag, message)

#ifdef LOGGING_DISABLED
#define LOG_F(level, tag, ...) \
  do {                         \
  }                            \
  CORE_LOOP_ONCE()
#else
/**
 * @brief Log a message using printf style formatting.
 * @param level Logging level.
 * @param tag Log component name.
 */
#define LOG_F(level, tag, ...)                        \
  do {                                                \
    if (::olp::logging::Log::isEnabled(level, tag)) { \
      DO_LOG_F(level, tag, __VA_ARGS__);              \
    }                                                 \
  }                                                   \
  CORE_LOOP_ONCE()

#endif  // LOGGING_DISABLED

#ifdef LOGGING_DISABLE_DEBUG_LEVEL
#define LOG_TRACE_F(tag, ...) CORE_UNUSED(tag, __VA_ARGS__)
#define LOG_DEBUG_F(tag, ...) CORE_UNUSED(tag, __VA_ARGS__)
#else
/**
 * @brief Log a trace message using printf style formatting.
 * @param tag Log component name.
 */
#define LOG_TRACE_F(tag, ...) \
  LOG_F(::olp::logging::Level::Trace, tag, __VA_ARGS__)

/**
 * @brief Log a debug message using printf style formatting.
 * @param tag Log component name.
 */
#define LOG_DEBUG_F(tag, ...) \
  LOG_F(::olp::logging::Level::Debug, tag, __VA_ARGS__)

#endif  // LOGGING_DISABLE_DEBUG_LEVEL

/**
 * @brief Log an info message using printf style formatting.
 * @param tag Log component name.
 */
#define LOG_INFO_F(tag, ...) \
  LOG_F(::olp::logging::Level::Info, tag, __VA_ARGS__)

/**
 * @brief Log a warning message using printf style formatting.
 * @param tag Log component name.
 */
#define LOG_WARNING_F(tag, ...) \
  LOG_F(::olp::logging::Level::Warning, tag, __VA_ARGS__)

/**
 * @brief Log an error message using printf style formatting.
 * @param tag Log component name.
 */
#define LOG_ERROR_F(tag, ...) \
  LOG_F(::olp::logging::Level::Error, tag, __VA_ARGS__)

/**
 * @brief Namespace for the logging library.
 */
namespace olp {
namespace logging {
class Configuration;
class FilterGroup;

/**
 * @brief The NullLogStream class
 *
 * This is used for disabled logs at compile time
 */
class NullLogStream {
 public:
  template <typename T>
  NullLogStream& operator<<(const T&) {
    return *this;
  }
};

/**
 * @brief Primary interface for logging messages.
 */
class CORE_API Log {
 public:
  /**
   * @brief Configures the logging system.
   *
   * This will override the previous configuration.
   * @return False if the configuration is invalid. The configuration will not
   * be changed in this case.
   */
  static bool configure(Configuration configuration);

  /**
   * @brief Gets a copy of the current configuration.
   *
   * This can be used to add an appender and re-configure the system.
   * @return A copy of the current configuration.
   */
  static Configuration getConfiguration();

  /**
   * @brief Sets the default log level.
   *
   * No messages below this level will be displayed unless overridden by
   * specific log tags.
   * @param level The log level.
   */
  static void setLevel(Level level);

  /**
   * @brief Gets the default log level.
   * @return The log level.
   */
  static Level getLevel();

  /**
   * @brief Sets the log level for a particular tag. This overrides the default.
   * @param level The log level.
   * @param tag Tag for the logging component. If empty, this will set the
   * default level.
   */
  static void setLevel(Level level, const std::string& tag);

  /**
   * @brief Sets the log level for a particular tag. This overrides the default.
   * @param level The log level.
   * @param tag Tag for the logging component. If empty, this will set the
   * default level.
   */
  static void setLevel(const std::string& level, const std::string& tag);

  /**
   * @brief Gets the log level for a particular tag.
   * @param tag Tag for the logging component. If empty, this will get the
   * default level.
   * @return The log level for the tag, or core::None if unset.
   */
  static boost::optional<Level> getLevel(const std::string& tag);

  /**
   * @brief Clears the log level for a particular tag, setting it to use the
   * default.
   * @param tag Tag for the logging component.
   */
  static void clearLevel(const std::string& tag);

  /**
   * @brief Clears the log levels for all tags, setting them to use the default.
   */
  static void clearLevels();

  /**
   * @brief Applies a filter group.
   *
   * This will clear all of the log levels for specific tags and replace them
   * with the levels set in the filter group. If the default log level is set in
   * the filter group, this will also be applied.
   * @param filters The filter group to apply.
   */
  static void applyFilterGroup(const FilterGroup& filters);

  /**
   * @brief Returns whether a specific level is enabled by default.
   * @param level The log level.
   * @return Whether or not the log is enabled.
   */
  static bool isEnabled(Level level);

  /**
   * @brief Returns whether or not a log tag is enabled for a specific level.
   * @param tag Tag for the logging component.
   * @param level The log level.
   * @return Whether or not the log is enabled.
   */
  static bool isEnabled(Level level, const std::string& tag);

  /**
   * @brief Logs a message to the registered appenders. Outputting to a specific
   * appender is dependant on whether the appender is enabled for this specific
   * level.
   * @param level Log level
   * @param tag Tag for the logging component.
   * @param message The log message
   * @param file Filename of the log caller
   * @param line Line of the log caller
   * @param function Function name of the log caller.
   * @param fullFunction The fully qualified function name of the log caller.
   */
  static void logMessage(Level level, const std::string& tag,
                         const std::string& message, const char* file,
                         unsigned int line, const char* function,
                         const char* fullFunction);
};

}  // namespace logging
}  // namespace olp
