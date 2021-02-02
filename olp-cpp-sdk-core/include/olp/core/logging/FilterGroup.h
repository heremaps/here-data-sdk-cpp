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

#include <istream>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/none.hpp>
#include <boost/optional.hpp>

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Level.h>

namespace olp {
namespace logging {
/**
 * @brief Class that groups together log levels for different tags. This makes
 * it easier to apply groups of level filters together.
 */
class CORE_API FilterGroup {
 public:
  inline FilterGroup();
  FilterGroup(const FilterGroup&) = default;
  FilterGroup& operator=(const FilterGroup&) = default;
  inline FilterGroup(FilterGroup&& other) noexcept;
  inline FilterGroup& operator=(FilterGroup&& other) noexcept;

  /**
   * @brief Gets the default log level.
   * @return The default log level, or boost::none if not set.
   */
  inline boost::optional<Level> getLevel() const;

  /**
   * @brief Sets the default log level.
   * @param level The default log level.
   */
  inline FilterGroup& setLevel(Level level);

  /**
   * @brief Clears the default log level to be unset.
   *
   * Leaving the default log level unset prevents the default level in Log from
   * changing when the filter group is applied.
   */
  inline FilterGroup& clearLevel();

  /**
   * @brief Gets the log level for a tag
   * @param tag The tag to get the log level for.
   * @return The log level for the tag, or boost::none if not set.
   */
  inline boost::optional<Level> getLevel(const std::string& tag) const;

  /**
   * @brief Sets the log level for a tag.
   * @param level The log level for a tag.
   * @param tag The tag to set the level for.
   */
  inline FilterGroup& setLevel(Level level, const std::string& tag);

  /**
   * @brief Clears the log level to be unset for a tag.
   *
   * Leaving the log level for a tag unset causes it to use the default log
   * level instead.
   */
  inline FilterGroup& clearLevel(const std::string& tag);

  /**
   * @brief Clears the filter group to be unset.
   */
  inline FilterGroup& clear();

  /**
   * @brief Loads the filter group from a file.
   * @param fileName The file to load the configuration from.
   * @return Whether or not the load was successful. If unsuccessful, this will
   * be cleared.
   */
  bool load(const std::string& fileName);

  /**
   * @brief Loads the filter group from a stream.
   *
   * The stream is expected contain text data. The format is as follows:
   *     - Blank lines, or lines that start with # are ignored.
   *     - Setting the log level for the tag is of the format "tag: level". For
   * example:
   *         - mylib: warning
   *         - theirlib: info
   *         - otherlib: off
   *     - Setting the default log level is of the format ": level". For
   * example: ": error"
   *     - Whitespace is trimmed and case is ignored for the levels.
   *
   * The filter group will be cleared before the contents of the stream are
   * applied.
   * @param stream The stream to load the configuration from.
   * @return Whether or not the load was successful. If unsuccessful, this will
   * be cleared.
   */
  bool load(std::istream& stream);

  /**
   * @brief Converts string logging level to enum Level format.
   * @param levelStr string level to convert.
   * @return Converted level
   */
  static boost::optional<Level> stringToLevel(const std::string& levelStr);

 private:
  friend class Log;

  boost::optional<Level> m_defaultLevel;
  std::unordered_map<std::string, Level> m_tagLevels;
};

inline FilterGroup::FilterGroup() : m_defaultLevel(boost::none) {}

inline FilterGroup::FilterGroup(FilterGroup&& other) noexcept
    : m_defaultLevel(std::move(other.m_defaultLevel)),
      m_tagLevels(std::move(other.m_tagLevels)) {}

inline FilterGroup& FilterGroup::operator=(FilterGroup&& other) noexcept {
  m_defaultLevel = std::move(other.m_defaultLevel);
  m_tagLevels = std::move(other.m_tagLevels);
  return *this;
}

inline boost::optional<Level> FilterGroup::getLevel() const {
  return m_defaultLevel;
}

inline FilterGroup& FilterGroup::setLevel(Level level) {
  m_defaultLevel = level;
  return *this;
}

inline FilterGroup& FilterGroup::clearLevel() {
  m_defaultLevel = boost::none;
  return *this;
}

inline boost::optional<Level> FilterGroup::getLevel(
    const std::string& tag) const {
  auto foundIter = m_tagLevels.find(tag);
  if (foundIter == m_tagLevels.end()) return boost::none;
  return foundIter->second;
}

inline FilterGroup& FilterGroup::setLevel(Level level, const std::string& tag) {
  m_tagLevels[tag] = level;
  return *this;
}

inline FilterGroup& FilterGroup::clearLevel(const std::string& tag) {
  m_tagLevels.erase(tag);
  return *this;
}

inline FilterGroup& FilterGroup::clear() {
  m_defaultLevel = boost::none;
  m_tagLevels.clear();
  return *this;
}

}  // namespace logging
}  // namespace olp
