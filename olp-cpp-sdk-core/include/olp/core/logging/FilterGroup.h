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

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Level.h>
#include <olp/core/porting/optional.hpp>

namespace olp {
namespace logging {
/**
 * @brief Groups together log levels for different tags.
 *
 * It helps to apply groups of level filters together.
 */
class CORE_API FilterGroup {
 public:
  inline FilterGroup();

  /// The default copy constructor.
  FilterGroup(const FilterGroup&) = default;

  /// The default copy assignment operator.
  FilterGroup& operator=(const FilterGroup&) = default;

  /// The default move constructor.
  inline FilterGroup(FilterGroup&& other) noexcept;

  /// The default move assignment operator.
  inline FilterGroup& operator=(FilterGroup&& other) noexcept;

  /**
   * @brief Gets the default log level.
   *
   * @return The default log level or `olp::porting::none` if the level is not set.
   */
  inline porting::optional<Level> getLevel() const;

  /**
   * @brief Sets the default log level.
   *
   * @param level The default log level.
   */
  inline FilterGroup& setLevel(Level level);

  /**
   * @brief Clears the default log level.
   *
   * If the default log level is unset, it does not change
   * when the filter group is applied.
   */
  inline FilterGroup& clearLevel();

  /**
   * @brief Gets the log level for a tag.
   *
   * @param tag The tag for which to get the log level.
   *
   * @return The log level for the tag, or `olp::porting::none` if the level is not set.
   */
  inline porting::optional<Level> getLevel(const std::string& tag) const;

  /**
   * @brief Sets the log level for a tag.
   *
   * @param level The log level for a tag.
   * @param tag The tag for which to set the level.
   */
  inline FilterGroup& setLevel(Level level, const std::string& tag);

  /**
   * @brief Clears the log level for a tag.
   *
   * If the log level for a tag is unset, the default log
   * level is used instead.
   */
  inline FilterGroup& clearLevel(const std::string& tag);

  /**
   * @brief Clears the filter group.
   */
  inline FilterGroup& clear();

  /**
   * @brief Loads the filter group from a file.
   *
   * @param fileName The file from which to load the configuration.
   *
   * @return True if the load was successful; false otherwise. If unsuccessful,
   * this is cleared.
   */
  bool load(const std::string& fileName);

  /**
   * @brief Loads the filter group from a stream.
   *
   * The stream should contain text data.
   * The format of the stream:
   * - Blank lines or lines that start with "#" are ignored.
   * - Use the following format for tag log levels: `tag: level`.
   * For example:
   *    - `mylib: warning`
   *    - `theirlib: info`
   *    - `otherlib: off`
   * - Use the following format for the default log level: `: level`.
   * For example: `: error`
   * - Whitespaces are trimmed.
   * - The case is ignored for levels.
   *
   * The filter groups are cleared before the content of the stream is
   * applied.
   *
   * @param stream The stream from which to load the configuration.
   *
   * @return True if the load was successful; false otherwise. If unsuccessful,
   * this is cleared.
   */
  bool load(std::istream& stream);

  /**
   * @brief Converts the string log level to the enum level format.
   *
   * @param levelStr The string level to convert.
   *
   * @return The converted level.
   */
  static porting::optional<Level> stringToLevel(const std::string& levelStr);

 private:
  friend class Log;

  porting::optional<Level> m_defaultLevel;
  std::unordered_map<std::string, Level> m_tagLevels;
};

inline FilterGroup::FilterGroup() : m_defaultLevel(porting::none) {}

inline FilterGroup::FilterGroup(FilterGroup&& other) noexcept
    : m_defaultLevel(std::move(other.m_defaultLevel)),
      m_tagLevels(std::move(other.m_tagLevels)) {}

inline FilterGroup& FilterGroup::operator=(FilterGroup&& other) noexcept {
  m_defaultLevel = std::move(other.m_defaultLevel);
  m_tagLevels = std::move(other.m_tagLevels);
  return *this;
}

inline porting::optional<Level> FilterGroup::getLevel() const {
  return m_defaultLevel;
}

inline FilterGroup& FilterGroup::setLevel(Level level) {
  m_defaultLevel = level;
  return *this;
}

inline FilterGroup& FilterGroup::clearLevel() {
  m_defaultLevel = porting::none;
  return *this;
}

inline porting::optional<Level> FilterGroup::getLevel(
    const std::string& tag) const {
  auto foundIter = m_tagLevels.find(tag);
  if (foundIter == m_tagLevels.end()) return porting::none;
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
  m_defaultLevel = porting::none;
  m_tagLevels.clear();
  return *this;
}

}  // namespace logging
}  // namespace olp
