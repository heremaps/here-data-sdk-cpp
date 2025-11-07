/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include <olp/core/logging/Log.h>

#include "ThreadId.h"

#include <olp/core/logging/Configuration.h>
#include <olp/core/logging/FilterGroup.h>
#include <olp/core/porting/optional.h>
#include <olp/core/thread/Atomic.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>

namespace olp {
namespace logging {

namespace {
constexpr auto kSecretMask = "*****";
const auto kSecretMaskLength = std::strlen(kSecretMask);
}  // namespace

struct LogMessageExt : public LogMessage {
  olp::porting::optional<std::string> adjusted_message;
};

class LogImpl {
 public:
  friend class olp::thread::Atomic<logging::LogImpl>;

  ~LogImpl();

  /**
   * @brief Access the alive status flag to query or set whether the LogImpl can
   * still be accessed.
   *
   * The LogImpl is a local static variable that resides inside the getInstance(
   * ) function. To avoid access to it once it is destroyed during the static
   * deinitialization phase, the alive status flag needs to be queried before
   * any access to the LogImpl is done. It is only set to false once the LogImpl
   * is being destroyed.
   */
  static bool& aliveStatus();
  static olp::thread::Atomic<LogImpl>& getInstance();

  bool configure(Configuration configuration);
  Configuration getConfiguration() const;

  void setLevel(Level level);
  Level getLevel() const;
  void setLevel(Level level, const std::string& tag);
  porting::optional<Level> getLevel(const std::string& tag) const;
  void clearLevel(const std::string& tag);
  void clearLevels();

  bool isEnabled(Level level) const;
  bool isEnabled(Level level, const std::string& tag) const;

  void logMessage(Level level, const std::string& tag,
                  const std::string& message, const char* file,
                  unsigned int line, const char* function,
                  const char* fullFunction);

  void addCensor(const std::string& msg);
  void removeCensor(const std::string& msg);

 private:
  LogImpl();
  template <class LogItem>
  void appendLogItem(const LogItem& log_item);

  template <class LogItem>
  void censorLogItem(LogItem& log_item, const std::string& original);

  Configuration m_configuration;
  std::unordered_map<std::string, Level> m_logLevels;
  Level m_defaultLevel;
  std::vector<std::string> m_toCensor;
};

LogImpl::LogImpl()
    : m_configuration(Configuration::createDefault()),
      m_defaultLevel(Level::Debug) {}

LogImpl::~LogImpl() { aliveStatus() = false; }

bool& LogImpl::aliveStatus() {
  static bool s_alive = true;

  return s_alive;
}

olp::thread::Atomic<LogImpl>& LogImpl::getInstance() {
  static olp::thread::Atomic<LogImpl> s_instance;

  return s_instance;
}

bool LogImpl::configure(Configuration configuration) {
  const bool is_valid = configuration.isValid();
  if (is_valid) {
    m_configuration = std::move(configuration);
  }

  return is_valid;
}

Configuration LogImpl::getConfiguration() const { return m_configuration; }

void LogImpl::setLevel(Level level) { m_defaultLevel = level; }

Level LogImpl::getLevel() const { return m_defaultLevel; }

void LogImpl::setLevel(Level level, const std::string& tag) {
  if (tag.empty()) {
    setLevel(level);
    return;
  }

  m_logLevels[tag] = level;
}

porting::optional<Level> LogImpl::getLevel(const std::string& tag) const {
  if (tag.empty())
    return getLevel();

  auto foundIter = m_logLevels.find(tag);
  if (foundIter == m_logLevels.end())
    return porting::none;
  else
    return foundIter->second;
}

void LogImpl::clearLevel(const std::string& tag) {
  if (tag.empty())
    return;

  m_logLevels.erase(tag);
}

void LogImpl::clearLevels() { m_logLevels.clear(); }

bool LogImpl::isEnabled(Level level) const {
  if (level == Level::Off)
    return false;

  return static_cast<int>(level) >= static_cast<int>(m_defaultLevel);
}

bool LogImpl::isEnabled(Level level, const std::string& tag) const {
  if (level == Level::Off)
    return false;

  auto foundIter = m_logLevels.find(tag);
  Level targetLevel;
  if (foundIter == m_logLevels.end())
    targetLevel = m_defaultLevel;
  else
    targetLevel = foundIter->second;
  return static_cast<int>(level) >= static_cast<int>(targetLevel);
}

void LogImpl::logMessage(Level level, const std::string& tag,
                         const std::string& message, const char* file,
                         unsigned int line, const char* function,
                         const char* fullFunction) {
  LogMessageExt logMessage;
  logMessage.level = level;
  logMessage.tag = tag.c_str();
  logMessage.message = message.c_str();
  logMessage.file = file;
  logMessage.line = line;
  logMessage.function = function;
  logMessage.fullFunction = fullFunction;
  logMessage.time = std::chrono::system_clock::now();
  logMessage.threadId = getThreadId();

  censorLogItem(logMessage, message);
  appendLogItem(logMessage);
}

template <class LogItem>
void LogImpl::appendLogItem(const LogItem& log_item) {
  for (const auto& appender_with_log_level : m_configuration.getAppenders()) {
    if (appender_with_log_level.isEnabled(log_item.level))
      appender_with_log_level.appender->append(log_item);
  }
}

template <class LogItem>
void LogImpl::censorLogItem(LogItem& log_item, const std::string& original) {
  bool has_copy = log_item.adjusted_message.has_value();
  const std::string& src =
      has_copy ? log_item.adjusted_message.value() : original;

  for (const std::string& secret : m_toCensor) {
    auto found_pos = src.find(secret);
    while (found_pos != std::string::npos) {
      if (!has_copy) {
        log_item.adjusted_message = original;
        has_copy = true;
        log_item.message = log_item.adjusted_message.value().c_str();
      }
      log_item.adjusted_message.value().replace(found_pos, secret.length(),
                                                kSecretMask);
      found_pos = log_item.adjusted_message.value().find(
          secret, found_pos + kSecretMaskLength);
    }
  }
}

void LogImpl::addCensor(const std::string& msg) {
  if (msg.empty()) {
    return;
  }

  m_toCensor.emplace_back(msg);
}

void LogImpl::removeCensor(const std::string& msg) {
  if (msg.empty()) {
    return;
  }

  auto it = std::find(m_toCensor.begin(), m_toCensor.end(), msg);
  if (it != m_toCensor.end()) {
    m_toCensor.erase(it);
  }
}

// implementation of public static Log API
//--------------------------------------------------------

bool Log::configure(Configuration configuration) {
  if (!LogImpl::aliveStatus())
    return false;

  return LogImpl::getInstance().locked([&configuration](LogImpl& log) {
    return log.configure(std::move(configuration));
  });
}

Configuration Log::getConfiguration() {
  if (!LogImpl::aliveStatus())
    return Configuration();

  return LogImpl::getInstance().locked(
      [](const LogImpl& log) { return log.getConfiguration(); });
}

void Log::setLevel(Level level) {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked([level](LogImpl& log) { log.setLevel(level); });
}

Level Log::getLevel() {
  if (!LogImpl::aliveStatus())
    return Level::Off;

  return LogImpl::getInstance().locked(
      [](const LogImpl& log) { return log.getLevel(); });
}

void Log::setLevel(Level level, const std::string& tag) {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked(
      [level, &tag](LogImpl& log) { log.setLevel(level, tag); });
}

void Log::setLevel(const std::string& level, const std::string& tag) {
  auto log_level = FilterGroup::stringToLevel(level);
  if (!log_level)
    return;

  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked(
      [&log_level, &tag](LogImpl& log) { log.setLevel(*log_level, tag); });
}

porting::optional<Level> Log::getLevel(const std::string& tag) {
  if (!LogImpl::aliveStatus())
    return porting::none;

  return LogImpl::getInstance().locked(
      [&tag](const LogImpl& log) { return log.getLevel(tag); });
}

void Log::clearLevel(const std::string& tag) {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked([&tag](LogImpl& log) { log.clearLevel(tag); });
}

void Log::clearLevels() {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked([](LogImpl& log) { log.clearLevels(); });
}

void Log::applyFilterGroup(const FilterGroup& filters) {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked([&filters](LogImpl& log) {
    log.clearLevels();
    log.clearLevels();
    auto defaultLevel = filters.getLevel();
    if (defaultLevel)
      log.setLevel(*defaultLevel);
    for (const auto& tagLevel : filters.m_tagLevels)
      log.setLevel(tagLevel.second, tagLevel.first);
  });
}

bool Log::isEnabled(Level level) {
  if (!LogImpl::aliveStatus())
    return false;

  return LogImpl::getInstance().locked(
      [level](const LogImpl& log) { return log.isEnabled(level); });
}

bool Log::isEnabled(Level level, const std::string& tag) {
  if (!LogImpl::aliveStatus())
    return false;

  return LogImpl::getInstance().locked(
      [level, &tag](const LogImpl& log) { return log.isEnabled(level, tag); });
}

void Log::logMessage(Level level, const std::string& tag,
                     const std::string& message, const char* file,
                     unsigned int line, const char* function,
                     const char* fullFunction) {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked([&](LogImpl& log) {
    log.logMessage(level, tag, message, file, line, function, fullFunction);
  });
}

void Log::addCensor(const std::string& message) {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked([&](LogImpl& log) { log.addCensor(message); });
}

void Log::removeCensor(const std::string& message) {
  if (!LogImpl::aliveStatus())
    return;

  LogImpl::getInstance().locked(
      [&](LogImpl& log) { log.removeCensor(message); });
}

}  // namespace logging
}  // namespace olp
