/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <cinttypes>
#include <cstdlib>
#include <sstream>
#include <string>

#include <olp/core/logging/Format.h>
#include <olp/core/logging/Level.h>

#include <olp/core/CoreApi.h>
#include <olp/core/porting/optional.h>
#include <olp/core/utils/WarningWorkarounds.h>

/**
 * @file
 * @brief Provides the main interface for the logging library.
 */

/**
 * @brief Gets the current function signature for different compilers.
 */
#ifdef LOGGING_DISABLE_LOCATION

#define OLP_SDK_LOG_FUNCTION_SIGNATURE ""
#define OLP_SDK_LOG_FILE ""
#define OLP_SDK_LOG_LINE 0
#define OLP_SDK_LOG_FUNCTION ""

#else  // LOGGING_DISABLE_LOCATION

#if __GNUC__ >= 3 || defined(__clang__)
#define OLP_SDK_LOG_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define OLP_SDK_LOG_FUNCTION_SIGNATURE __FUNCSIG__
#else
#define OLP_SDK_LOG_FUNCTION_SIGNATURE __FUNCTION__
#endif

#define OLP_SDK_LOG_FILE __FILE__
#define OLP_SDK_LOG_LINE __LINE__
#define OLP_SDK_LOG_FUNCTION __FUNCTION__

#endif  // LOGGING_DISABLE_LOCATION

/**
 * @brief Logs a message using C++ style streams.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param level The log level.
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_DO_LOG(level, tag, message)                           \
  do {                                                                \
    std::ostringstream __strm;                                        \
    __strm << message;                                                \
    ::olp::logging::Log::logMessage(                                  \
        level, tag, __strm.str(), OLP_SDK_LOG_FILE, OLP_SDK_LOG_LINE, \
        OLP_SDK_LOG_FUNCTION, OLP_SDK_LOG_FUNCTION_SIGNATURE);        \
  }                                                                   \
  OLP_SDK_CORE_LOOP_ONCE()

/**
 * @brief Logs a "Critical" message using C++ style streams.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param level The log level.
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_CRITICAL(level, tag, message) \
  OLP_SDK_DO_LOG(level, tag, message)

/**
 * @brief Logs a "Critical info" message using C++ style streams.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_CRITICAL_INFO(tag, message) \
  OLP_SDK_LOG_CRITICAL(::olp::logging::Level::Info, tag, message)

/**
 * @brief Logs a "Critical warning" message using C++ style streams.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_CRITICAL_WARNING(tag, message) \
  OLP_SDK_LOG_CRITICAL(::olp::logging::Level::Warning, tag, message)

/**
 * @brief Logs a "Critical error" message using C++ style streams.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_CRITICAL_ERROR(tag, message) \
  OLP_SDK_LOG_CRITICAL(::olp::logging::Level::Error, tag, message)

/**
 * @brief Logs a "Fatal error" message using C++ style streams.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_FATAL(tag, message) \
  OLP_SDK_LOG_CRITICAL(::olp::logging::Level::Fatal, tag, message)

/**
 * @brief Logs a "Critical fatal error" message using C++ style streams,
 * and then aborts the program.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_ABORT(tag, message) \
  do {                                  \
    OLP_SDK_LOG_FATAL(tag, message);    \
    std::abort();                       \
  }                                     \
  OLP_SDK_CORE_LOOP_ONCE()

/**
 * @brief Logs a message using the printf-style formatting.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param level The log level.
 * @param tag The tag for the log component.
 */
#define OLP_SDK_DO_LOG_F(level, tag, ...)                                    \
  do {                                                                       \
    std::string __message = ::olp::logging::format(__VA_ARGS__);             \
    ::olp::logging::Log::logMessage(level, tag, __message, OLP_SDK_LOG_FILE, \
                                    OLP_SDK_LOG_LINE, OLP_SDK_LOG_FUNCTION,  \
                                    OLP_SDK_LOG_FUNCTION_SIGNATURE);         \
  }                                                                          \
  OLP_SDK_CORE_LOOP_ONCE()

/**
 * @brief Logs a "Critical" message using the printf-style formatting.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param level The log level.
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_CRITICAL_F(level, tag, ...) \
  OLP_SDK_DO_LOG_F(level, tag, __VA_ARGS__)

/**
 * @brief Logs a "Critical info" message using the printf-style formatting.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_CRITICAL_INFO_F(tag, ...) \
  OLP_SDK_LOG_CRITICAL_F(::olp::logging::Level::Info, tag, __VA_ARGS__)

/**
 * @brief Logs a "Critical warning" message using the printf-style formatting.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_CRITICAL_WARNING_F(tag, ...) \
  OLP_SDK_LOG_CRITICAL_F(::olp::logging::Level::Warning, tag, __VA_ARGS__)

/**
 * @brief Logs a "Critical error" message using the printf-style formatting.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_CRITICAL_ERROR_F(tag, ...) \
  OLP_SDK_LOG_CRITICAL_F(::olp::logging::Level::Error, tag, __VA_ARGS__)

/**
 * @brief Logs a "Critical fatal error" message using the printf-style
 * formatting.
 *
 * `OLP_SDK_LOGGING_DISABLED` does not disable this functionality.
 * Additionally, it does not check to see if the tag is disabled.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_FATAL_F(tag, ...) \
  OLP_SDK_LOG_CRITICAL_F(::olp::logging::Level::Fatal, tag, __VA_ARGS__)

/**
 * @brief Logs a "Critical fatal error" message using the printf-style
 * formatting, and then abort sthe program.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_ABORT_F(tag, ...)      \
  do {                                     \
    OLP_SDK_LOG_FATAL_F(tag, __VA_ARGS__); \
    std::abort();                          \
  }                                        \
  OLP_SDK_CORE_LOOP_ONCE()

#ifdef OLP_SDK_LOGGING_DISABLED
#define OLP_SDK_LOG(level, tag, message) \
  do {                                   \
  }                                      \
  OLP_SDK_CORE_LOOP_ONCE()
#else

/**
 * @brief Logs a message using C++ style streams.
 *
 * @param level The log level.
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG(level, tag, message)              \
  do {                                                \
    if (::olp::logging::Log::isEnabled(level, tag)) { \
      OLP_SDK_DO_LOG(level, tag, message);            \
    }                                                 \
  }                                                   \
  OLP_SDK_CORE_LOOP_ONCE()

#endif  // OLP_SDK_LOGGING_DISABLED

#ifdef LOGGING_DISABLE_DEBUG_LEVEL
#define OLP_SDK_LOG_TRACE(tag, message)   \
  do {                                    \
    ::olp::logging::NullLogStream __strm; \
    __strm << message;                    \
    OLP_SDK_CORE_UNUSED(tag, __strm);     \
  }                                       \
  OLP_SDK_CORE_LOOP_ONCE()

#define OLP_SDK_LOG_DEBUG(tag, message)   \
  do {                                    \
    ::olp::logging::NullLogStream __strm; \
    __strm << message;                    \
    OLP_SDK_CORE_UNUSED(tag, __strm);     \
  }                                       \
  OLP_SDK_CORE_LOOP_ONCE()

#else
/**
 * @brief Logs a "Trace" message using C++ style streams.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_TRACE(tag, message) \
  OLP_SDK_LOG(::olp::logging::Level::Trace, tag, message)

/**
 * @brief Logs a "Debug" message using C++ style streams.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_DEBUG(tag, message) \
  OLP_SDK_LOG(::olp::logging::Level::Debug, tag, message)

#endif  // LOGGING_DISABLE_DEBUG_LEVEL

/**
 * @brief Logs an "Info" message using C++ style streams.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_INFO(tag, message) \
  OLP_SDK_LOG(::olp::logging::Level::Info, tag, message)

/**
 * @brief Logs a "Warning" message using C++ style streams.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_WARNING(tag, message) \
  OLP_SDK_LOG(::olp::logging::Level::Warning, tag, message)

/**
 * @brief Logs an "Error" message using C++ style streams.
 *
 * @param tag The tag for the log component.
 * @param message The log message.
 */
#define OLP_SDK_LOG_ERROR(tag, message) \
  OLP_SDK_LOG(::olp::logging::Level::Error, tag, message)

#ifdef OLP_SDK_LOGGING_DISABLED
#define OLP_SDK_LOG_F(level, tag, ...) \
  do {                                 \
  }                                    \
  OLP_SDK_CORE_LOOP_ONCE()
#else
/**
 * @brief Logs a message using the printf style formatting.
 *
 * @param level The log level.
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_F(level, tag, ...)                \
  do {                                                \
    if (::olp::logging::Log::isEnabled(level, tag)) { \
      OLP_SDK_DO_LOG_F(level, tag, __VA_ARGS__);      \
    }                                                 \
  }                                                   \
  OLP_SDK_CORE_LOOP_ONCE()

#endif  // OLP_SDK_LOGGING_DISABLED

#ifdef LOGGING_DISABLE_DEBUG_LEVEL
#define OLP_SDK_LOG_TRACE_F(tag, ...) OLP_SDK_CORE_UNUSED(tag, __VA_ARGS__)
#define OLP_SDK_LOG_DEBUG_F(tag, ...) OLP_SDK_CORE_UNUSED(tag, __VA_ARGS__)
#else
/**
 * @brief Logs a "Trace" message using the printf style formatting.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_TRACE_F(tag, ...) \
  OLP_SDK_LOG_F(::olp::logging::Level::Trace, tag, __VA_ARGS__)

/**
 * @brief Logs a "Debug" message using the printf style formatting.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_DEBUG_F(tag, ...) \
  OLP_SDK_LOG_F(::olp::logging::Level::Debug, tag, __VA_ARGS__)

#endif  // LOGGING_DISABLE_DEBUG_LEVEL

/**
 * @brief Logs an "Info" message using the printf style formatting.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_INFO_F(tag, ...) \
  OLP_SDK_LOG_F(::olp::logging::Level::Info, tag, __VA_ARGS__)

/**
 * @brief Logs a "Warning" message using the printf style formatting.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_WARNING_F(tag, ...) \
  OLP_SDK_LOG_F(::olp::logging::Level::Warning, tag, __VA_ARGS__)

/**
 * @brief Logs an "Eror" message using the printf style formatting.
 *
 * @param tag The tag for the log component.
 */
#define OLP_SDK_LOG_ERROR_F(tag, ...) \
  OLP_SDK_LOG_F(::olp::logging::Level::Error, tag, __VA_ARGS__)

/**
 * @brief A namespace for the logging library.
 */
namespace olp {
namespace logging {
class Configuration;
class FilterGroup;

/**
 * @brief Used for disabled logs at compile time.
 */
class NullLogStream {
 public:
  template <typename T>

  /**
   * @brief The stream operator to print or serialize the given log stream.
   */
  NullLogStream& operator<<(const T&) {
    return *this;
  }
};

/**
 * @brief A primary interface for log messages.
 */
class CORE_API Log {
 public:
  /**
   * @brief Configures the log system.
   *
   * It overrides the previous configuration.
   *
   * @return False if the configuration is invalid.
   * The configuration does not change in this case.
   */
  static bool configure(Configuration configuration);

  /**
   * @brief Gets a copy of the current configuration.
   *
   * Use it to add an appender and reconfigure the system.
   *
   * @return A copy of the current configuration.
   */
  static Configuration getConfiguration();

  /**
   * @brief Sets the default log level.
   *
   * No messages below this level are displayed unless overridden by
   * specific log tags.
   *
   * @param level The log level.
   */
  static void setLevel(Level level);

  /**
   * @brief Gets the default log level.
   *
   * @return The log level.
   */
  static Level getLevel();

  /**
   * @brief Sets the log level for a tag.
   *
   * It overrides the default configurations.
   *
   * @param level The log level.
   * @param tag The tag for the log component. If empty, it sets the
   * default level.
   */
  static void setLevel(Level level, const std::string& tag);

  /**
   * @brief Sets the log level for a tag.
   *
   * It overrides the default configurations.
   *
   * @param level The log level.
   * @param tag The tag for the log component. If empty, it sets the
   * default level.
   */
  static void setLevel(const std::string& level, const std::string& tag);

  /**
   * @brief Gets the log level for a tag.
   *
   * @param tag The tag for the log component. If empty, it sets the
   * default level.
   *
   * @return The log level for the tag or `core::None` if the log level is
   * unset.
   */
  static porting::optional<Level> getLevel(const std::string& tag);

  /**
   * @brief Clears the log level for a tag and sets it to
   * the default value.
   *
   * @param tag The tag for the log component.
   */
  static void clearLevel(const std::string& tag);

  /**
   * @brief Clears the log levels for all tags and sets them to
   * the default value.
   */
  static void clearLevels();

  /**
   * @brief Applies a filter group.
   *
   * It clears all the log levels for tags and replaces them
   * with the levels set in the filter group. If the default log level is set in
   * the filter group, it is also applied.
   *
   * @param filters The filter group to apply.
   */
  static void applyFilterGroup(const FilterGroup& filters);

  /**
   * @brief Checks whether a level is enabled by default.
   *
   * @param level The log level.
   *
   * @return True if the log is enabled; false otherwise.
   */
  static bool isEnabled(Level level);

  /**
   * @brief Checks whether a log tag is enabled for a level.
   *
   * @param tag The tag for the log component.
   * @param level The log level.
   *
   * @return True if the log is enabled; false otherwise.
   */
  static bool isEnabled(Level level, const std::string& tag);

  /**
   * @brief Logs a message to the registered appenders.
   *
   * Outputting to the appender depends on whether
   * the appender is enabled for this level.
   *
   * @param level The log level.
   * @param tag The tag for the log component.
   * @param message The log message.
   * @param file The file that generated the message.
   * @param line The line in the file where the message was logged.
   * @param function The function that generated the message.
   * @param fullFunction The fully qualified function that generated the
   * message.
   */
  static void logMessage(Level level, const std::string& tag,
                         const std::string& message, const char* file,
                         unsigned int line, const char* function,
                         const char* fullFunction);
};
}  // namespace logging
}  // namespace olp
