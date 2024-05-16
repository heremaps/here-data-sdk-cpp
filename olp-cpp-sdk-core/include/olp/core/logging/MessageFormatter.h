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

#include <array>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/CoreApi.h>

#include <olp/core/logging/Format.h>
#include <olp/core/logging/Level.h>
#include <olp/core/logging/LogMessage.h>

namespace olp {
namespace logging {
/**
 * @brief Specifies how messages are formatted.
 *
 * It provides a common way to declare any message format, which can be
 * re-used across appenders that utilize this class.
 *
 * Example: if you want the message to be set to "LOG: level tag - file:line
 * [time] message" where the file is limited to 30 characters (cutting off on
 * the left), the line always prints up to 5 characters, and time is set to HH:MM in UTC
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
   * @brief Maps the log level to its name.
   *
   * Casts the log level enum value to `size_t` to use as an index.
   * `Off` is not a valid name.
   */
  using LevelNameMap = std::array<std::string, levelCount>;

  /**
   * @brief The type of the element to print out.
   */
  enum class ElementType {
    String,   ///< The string literal.
    Level,    ///< The log level for the message. It is formatted as a string.
    Tag,      ///< The tag for the log component. It is formatted as a string. This
              ///< element is
              ///  omitted if the tag is null or an empty string.
    Message,  ///< The log message. It is formatted as a string.
    File,     ///< The file that generated the message. It is formatted as a
              ///< string.
    Line,     ///< The line in the file where the message was logged. It is formatted as an
              ///< unsigned integer.
    Function,  ///< The function that generated the message. It is formatted
               ///< as a string.
    FullFunction,  ///< The fully qualified function that generated the message.
                   ///< It is
                   ///  formatted as a string.
    Time,    ///< The timestamp of the message. It is formatted as a time using
             ///  `strftime()`.
    TimeMs,  ///< The millisecond portion of the timestamp. It is formatted as
             ///< an unsigned integer.
    ThreadId, ///< The thread ID of the thread that generated the message.
              ///< It is formatted as an unsigned long.
    ContextValue,  ///< A key/value literal from LogContext; 'format' is the key to look up.
  };

  /**
   * @brief An element to print out in the final formatted message.
   */
  struct CORE_API Element {
    /**
     * @brief Creates an `Element` instance with the element type.
     *
     * The format string is set automatically based on the type.
     *
     * @param type_ The element type.
     */
    explicit Element(ElementType type_);

    /**
     * @brief Creates an `Element` instance with all of the members.
     *
     * @param type_ The element type.
     * @param format_ The format string. It is a literal when `type_` is
     * `ElementType::String`.
     * @param limit_ The number of characters to limit string types before
     * passing to the formatter. A negative number cuts off from the
     * beginning of the string. A positive number cuts off from the end
     * of the string. A value of zero leaves the input string untouched.
     */
    inline Element(ElementType type_, std::string format_, int limit_ = 0);

    /// The default copy constructor.
    Element(const Element&) = default;

    /// The default copy operator.
    Element& operator=(const Element&) = default;

    /// The move operator overload.
    inline Element(Element&& other) noexcept;

    /// The move assignment operator overload.
    inline Element& operator=(Element&& other) noexcept;

    /**
     * @brief Checks whether the values of two `Element` instances
     * are the same.
     *
     * @param other The other `Element` instance.
     *
     * @return True if the values are the same; false otherwise.
     */
    inline bool operator==(const Element& other) const;

    /**
     * @brief Checks whether the values of two `Element` instances
     * are not the same.
     * 
     * @param other The other `Element` instance.
     * 
     * @return True if the values are not the same; false otherwise.
     */
    inline bool operator!=(const Element& other) const;

    /**
     * @brief The type of element to print.
     */
    ElementType type;

    /**
     * @brief The format for printing out the element.
     *
     * It is used as a literal for `ElementType::String` without passing through
     * `printf`.
     */
    std::string format;

    /**
     * @brief The number of characters to limit string types before passing to
     * the formatter.
     *
     * A negative number cuts off from the beginning of a string.
     * A positive number cuts off from the end of the string. A value of zero
     * leaves the input string untouched.
     */
    int limit;
  };

  /**
   * @brief The timezone used to print timestamps.
   */
  enum class Timezone {
    Local,  ///< Prints time in the local time standard.
    Utc     ///< Prints time in the UTC standard.
  };

  /**
   * @brief Gets the default level name map.
   *
   * @return The level name map.
   */
  static const LevelNameMap& defaultLevelNameMap();

  /**
   * @brief Creates the default message formatter.
   *
   * Format: "level tag - message".
   */
  static MessageFormatter createDefault();

  /**
   * @brief The default constructor.
   *
   * The element list is empty, the level name map is set to default,
   * and the timezone is set to local.
   */
  inline MessageFormatter();

  /**
   * @brief Creates the `MessageFormatter` instance with the elements, level name mapping, and timezone.
   *
   * @param elements The elements for the formatted message.
   * @param levelNameMap The level name map.
   * @param timezone The timezone for timestamps. Defaults to local time.
   */
  explicit inline MessageFormatter(
      std::vector<Element> elements,
      LevelNameMap levelNameMap = defaultLevelNameMap(),
      Timezone timezone = Timezone::Local);

  /// The default copy constructor.
  MessageFormatter(const MessageFormatter&) = default;

  /// The default copy assignment operator.
  MessageFormatter& operator=(const MessageFormatter&) = default;

  /// The move constructor overload.
  inline MessageFormatter(MessageFormatter&& other) noexcept;

  /// The default move assignment operator overload.
  inline MessageFormatter& operator=(MessageFormatter&& other) noexcept;

  /**
   * @brief Gets the elements for the format.
   *
   * @return The elements.
   */
  inline const std::vector<Element>& getElements() const;

  /**
   * @brief Sets the elements for the format.
   *
   * @param elements The elements.
   */
  inline MessageFormatter& setElements(std::vector<Element> elements);

  /**
   * @brief Gets the level name map.
   *
   * @return The level name map.
   */
  inline const LevelNameMap& getLevelNameMap() const;

  /**
   * @brief Sets the level name map.
   *
   * @param map The level name map.
   */
  inline MessageFormatter& setLevelNameMap(LevelNameMap map);

  /**
   * @brief Gets the timezone for timestamps.
   *
   * @return The timezone.
   */
  inline Timezone getTimezone() const;

  /**
   * @brief Sets the timezone for timestamps.
   *
   * @param timezone The timezone.
   */
  inline MessageFormatter& setTimezone(Timezone timezone);

  /**
   * @brief Formats a log message.
   *
   * @param message The message to format.
   *
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

inline MessageFormatter& MessageFormatter::setElements(
    std::vector<Element> elements) {
  m_elements = std::move(elements);
  return *this;
}

inline const MessageFormatter::LevelNameMap& MessageFormatter::getLevelNameMap()
    const {
  return m_levelNameMap;
}

inline MessageFormatter& MessageFormatter::setLevelNameMap(LevelNameMap map) {
  m_levelNameMap = std::move(map);
  return *this;
}

inline MessageFormatter::Timezone MessageFormatter::getTimezone() const {
  return m_timezone;
}

inline MessageFormatter& MessageFormatter::setTimezone(Timezone timezone) {
  m_timezone = timezone;
  return *this;
}

}  // namespace logging
}  // namespace olp
