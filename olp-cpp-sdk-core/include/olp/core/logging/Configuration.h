/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <memory>
#include <utility>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Appender.h>
#include <olp/core/logging/MessageFormatter.h>

namespace olp {
namespace logging {
/**
 * @brief Configures appenders and loggers available in
 * the logging system.
 */
class CORE_API Configuration {
 public:
  /**
   * @brief Contains an appender and its log level.
   */
  struct AppenderWithLogLevel {
    /**
     * @brief The log level of the appender.
     *
     * Any log level that is less than this level is ignored.
     */
    logging::Level logLevel;

    /**
     * @brief The appender.
     */
    std::shared_ptr<IAppender> appender;

    /**
     * @brief Checks whether the appender is enabled for the log level.
     *
     * @param level The log level.
     */
    bool isEnabled(logging::Level level) const {
      return appender && static_cast<int>(level) >= static_cast<int>(logLevel);
    }
  };

  /**
   * @brief A typedef for a list of appenders.
   */
  using AppenderList = std::vector<AppenderWithLogLevel>;

  /**
   * @brief Creates a default configuration by adding
   * an instance of `DebugAppender` and `ConsoleAppender` as appenders.
   *
   * @return The default configuration.
   */
  static Configuration createDefault();

  /**
   * @brief Checks whether the configuration is valid.
   *
   * @return True if the configuration is valid; false otherwise.
   */
  inline bool isValid() const;

  /**
   * @brief Adds the appender along with its log level to the configuration.
   *
   * @param appender The appender to add.
   * @param level The log level of the appender.
   */
  inline Configuration& addAppender(
      std::shared_ptr<IAppender> appender,
      logging::Level level = logging::Level::Trace);

  /**
   * @brief Clears the list of appenders.
   */
  inline Configuration& clear();

  /**
   * @brief Gets the appenders from the configuration.
   *
   * @return The appenders and their log levels.
   */
  inline const AppenderList& getAppenders() const;

 private:
  AppenderList m_appenders;
};

inline bool Configuration::isValid() const { return !m_appenders.empty(); }

inline Configuration& Configuration::addAppender(
    std::shared_ptr<IAppender> appender, logging::Level level) {
  if (appender) {
    AppenderWithLogLevel appenderWithLogLevel = {level, std::move(appender)};
    m_appenders.emplace_back(std::move(appenderWithLogLevel));
  }
  return *this;
}

inline Configuration& Configuration::clear() {
  m_appenders.clear();
  return *this;
}

inline auto Configuration::getAppenders() const -> const AppenderList& {
  return m_appenders;
}

}  // namespace logging
}  // namespace olp
