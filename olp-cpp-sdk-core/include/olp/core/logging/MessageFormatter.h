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

#include <olp/core/logging/Format.h>
#include <olp/core/logging/Level.h>
#include <olp/core/logging/LogMessage.h>
#include <array>
#include <string>
#include <vector>

namespace olp {
namespace logging {
/**
 * @brief Utility for specifying how messages are formatted.
 *
 * This provides a common way to declare any message format, which can be
 * re-used across appenders that utilize this class.
 *
 * Example: if you want the message to be set to "LOG: level tag - file:line
 * [time] message", where the file is limited to 30 characters (cutting off on
 * the left), line always prints to 5 characters, and time set to HH:MM in UTC
 * time:
 *
 * @code
 * MessageFormatter formatter(
 *     {
 *         MessageFormatter::Element(MessageFormatter::ElementType::String,
 * "LOG: "), MessageFormatter::Element(MessageFormatter::ElementType::Level, "%s
 * "), MessageFormatter::Element(MessageFormatter::ElementType::Tag, "%s - "),
 *         MessageFormatter::Element(MessageFormatter::ElementType::File, "%s:",
 * -30), MessageFormatter::Element(MessageFormatter::ElementType::Line, "%5u "),
 *         MessageFormatter::Element(MessageFormatter::ElementType::Time,
 * "[%H:%M] "),
 *         MessageFormatter::Element(MessageFormatter::ElementType::Message)
 *     }, MessageFormatter::defaultLevelNameMap(),
 * MessageFormatter::Timezone::Utc);
 * @endcode
 */
class CORE_API MessageFormatter {
 public:
  /**
   * @brief A mapping from the log level to the name to use.
   *
   * Cast the log level enum value to size_t to use as an index. Off is not a
   * valid name.
   */
  using LevelNameMap = std::array<std::string, levelCount>;

  /**
   * @brief The type of the element to print out.
   */
  enum class ElementType {
    String,   ///< A string literal.
    Level,    ///< The log level for the message. This is formatted as a string.
    Tag,      ///< The tag for the message. This is formatted as a string. This
              ///< element will
              ///  be ommitted if the tag is null or an empty string.
    Message,  ///< The message for the log. This is formatted as a string.
    File,     ///< The file that generated the message. This is formatted as a
              ///< string.
    Line,     ///< The line that generated the message. This is formatted as an
              ///< unsigned int.
    Function,  ///< The function that generated the message. This is formatted
               ///< as a string.
    FullFunction,  ///< The fully qualified function that generated the message.
                   ///< This is
                   ///  formatted as a string.
    Time,    ///< The timestamp of the message. This is formatted as a time as
             ///< with
             ///  strftime().
    TimeMs,  ///< The millisecond portion of the timestamp. This is formatted as
             ///< an
             ///  unsigned int.
    ThreadId  ///< The thread ID for the thread that generated the message. This
              ///< is
              ///  formatted as an unsigned long.
  };

  /**
   * @brief An element to print out in the final formatted message.
   */
  struct CORE_API Element {
    /**
     * @brief Constructs this with the element type.
     *
     * The format string will be set automatically based on the type.
     *
     * @param type_ The element type.
     */
    explicit Element(ElementType type_);

    /**
     * @brief Constructs this with all of the members.
     * @param type_ The element type.
     * @param format_ The format string. This is a literal when type_ is
     * ElementType::String.
     * @param limit_ The number of characters to limit string types before
     * passing to the formatter. A negative number will cut off from the
     * beginning of a string, while a positive number will cut off from the end
     * of the string. A value of zero will leave the input string untouched.
     */
    inline Element(ElementType type_, std::string format_, int limit_ = 0);

    Element(const Element&) = default;
    Element& operator=(const Element&) = default;

    inline Element(Element&& other) noexcept;
    inline Element& operator=(Element&& other) noexcept;

    inline bool operator==(const Element& other) const;
    inline bool operator!=(const Element& other) const;

    /**
     * @brief The type of element to print.
     */
    ElementType type;

    /**
     * @brief The format string for printing out the element.
     *
     * This is used as a literal for ElementType::String without passing through
     * printf.
     */
    std::string format;

    /**
     * @brief The number of characters to limit string types before passing to
     * the formatter.
     *
     * A negative number will cut off from the beginning of a string, while a
     * positive number will cut off from the end of the string. A value of zero
     * will leave the input string untouched.
     */
    int limit;
  };

  /**
   * @brief Enum for the timezone when printing timestamps.
   */
  enum class Timezone {
    Local,  ///< Print times in local time.
    Utc     ///< Print times in UTC.
  };

  /**
   * @brief Gets the default level name map.
   * @return The level name map.
   */
  static const LevelNameMap& defaultLevelNameMap();

  /**
   * @brief Makes the default message formatter.
   *
   * This will be of the format "level tag - message".
   */
  static MessageFormatter createDefault();

  /**
   * @brief Default constructor.
   *
   * The element list will be empty, the level name map will be set to default,
   * and the timezone will be set to local.
   */
  inline MessageFormatter();

  /**
   * @brief Constructs this with the elements, level name mapping, and timezone.
   * @param elements The elements for the formatted message.
   * @param levelNameMap The level name map.
   * @param timezone The timezone for timestamps. Defaults to local time.
   */
  explicit inline MessageFormatter(
      std::vector<Element> elements,
      LevelNameMap levelNameMap = defaultLevelNameMap(),
      Timezone timezone = Timezone::Local);

  MessageFormatter(const MessageFormatter&) = default;
  MessageFormatter& operator=(const MessageFormatter&) = default;

  inline MessageFormatter(MessageFormatter&& other) noexcept;
  inline MessageFormatter& operator=(MessageFormatter&& other) noexcept;

  /**
   * @brief Gets the elements for the format.
   * @return The elements.
   */
  inline const std::vector<Element>& getElements() const;

  /**
   * @brief Sets the elements for the format.
   * @param elements The elements.
   */
  inline void setElements(std::vector<Element> elements);

  /**
   * @brief Gets the level name map.
   * @return The level name map.
   */
  inline const LevelNameMap& getLevelNameMap() const;

  /**
   * @brief Sets the level name map.
   * @param map The level name map.
   */
  inline void setLevelNameMap(LevelNameMap map);

  /**
   * @brief Gets the timezone for timestamps.
   * @return The timezone.
   */
  inline Timezone getTimezone() const;

  /**
   * @brief Sets the timezone for timestamps.
   * @param timezone The timezone.
   */
  inline void setTimezone(Timezone timezone);

  /**
   * @brief Formats a log message.
   * @param message The message to format.
   * @return The formatted message.
   */
  std::string format(const LogMessage& message) const;

 private:
  std::vector<Element> m_elements;
  LevelNameMap m_levelNameMap;
  Timezone m_timezone;
};

inline MessageFormatter::Element::Element(ElementType type_,
                                          std::string format_, int limit_)
    : type(type_), format(std::move(format_)), limit(limit_) {}

inline MessageFormatter::Element::Element(Element&& other) noexcept
    : type(other.type), format(std::move(other.format)), limit(other.limit) {}

inline MessageFormatter::Element& MessageFormatter::Element::operator=(
    Element&& other) noexcept {
  type = other.type;
  format = std::move(other.format);
  limit = other.limit;
  return *this;
}

inline bool MessageFormatter::Element::operator==(const Element& other) const {
  return type == other.type && format == other.format && limit == other.limit;
}

inline bool MessageFormatter::Element::operator!=(const Element& other) const {
  return !(*this == other);
}

inline MessageFormatter::MessageFormatter()
    : m_levelNameMap(defaultLevelNameMap()), m_timezone(Timezone::Local) {}

inline MessageFormatter::MessageFormatter(std::vector<Element> elements,
                                          LevelNameMap levelNameMap,
                                          Timezone timezone)
    : m_elements(std::move(elements)),
      m_levelNameMap(std::move(levelNameMap)),
      m_timezone(timezone) {}

inline MessageFormatter::MessageFormatter(MessageFormatter&& other) noexcept
    : m_elements(std::move(other.m_elements)),
      m_levelNameMap(std::move(other.m_levelNameMap)),
      m_timezone(other.m_timezone) {}

inline MessageFormatter& MessageFormatter::operator=(
    MessageFormatter&& other) noexcept {
  m_elements = std::move(other.m_elements);
  m_levelNameMap = std::move(other.m_levelNameMap);
  m_timezone = other.m_timezone;
  return *this;
}

inline const std::vector<MessageFormatter::Element>&
MessageFormatter::getElements() const {
  return m_elements;
}

inline void MessageFormatter::setElements(std::vector<Element> elements) {
  m_elements = std::move(elements);
}

inline const MessageFormatter::LevelNameMap& MessageFormatter::getLevelNameMap()
    const {
  return m_levelNameMap;
}

inline void MessageFormatter::setLevelNameMap(LevelNameMap map) {
  m_levelNameMap = std::move(map);
}

inline MessageFormatter::Timezone MessageFormatter::getTimezone() const {
  return m_timezone;
}

inline void MessageFormatter::setTimezone(Timezone timezone) {
  m_timezone = timezone;
}

}  // namespace logging
}  // namespace olp
