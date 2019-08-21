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
#include <olp/core/logging/MessageFormatter.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <string>

namespace olp {
namespace logging {
static const char* limitString(std::string& tempStr, const char* str,
                               int limit) {
  if (limit < 0) {
    int length = static_cast<int>(std::strlen(str));
    int start = std::max(length + limit, 0);
    if (start == 0) return str;

    if (length - start > 3) {
      tempStr = "...";
      start += 3;
    } else
      tempStr.clear();

    tempStr.append(str, start, length - start);
    return tempStr.c_str();
  } else if (limit > 0) {
    std::size_t strLen = std::strlen(str);
    std::size_t finalLength = std::min(strLen, static_cast<std::size_t>(limit));
    if (finalLength == strLen) return str;

    bool addElipses = false;
    if (finalLength > 3) {
      finalLength -= 3;
      addElipses = true;
    }

    tempStr.assign(str, finalLength);
    if (addElipses) tempStr += "...";
    return tempStr.c_str();
  } else
    return str;
}

MessageFormatter::Element::Element(ElementType type_) : type(type_), limit(0) {
  switch (type) {
    case ElementType::Level:
    case ElementType::Tag:
    case ElementType::Message:
    case ElementType::File:
    case ElementType::Function:
    case ElementType::FullFunction:
      format = "%s";
      break;
    case ElementType::Line:
      format = "%u";
      break;
    case ElementType::Time:
      format = "%Y-%m-%d %H:%M:%S";
      break;
    case ElementType::TimeMs:
      format = "%u";
      break;
    case ElementType::ThreadId:
      format = "%lu";
      break;
    default:
      break;
  }
}

const MessageFormatter::LevelNameMap& MessageFormatter::defaultLevelNameMap() {
  static MessageFormatter::LevelNameMap defaultLevelNameMap = {
      {"[TRACE]", "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]", "[FATAL]"}};

  return defaultLevelNameMap;
}

MessageFormatter MessageFormatter::createDefault() {
  return MessageFormatter({Element(ElementType::Level, "%s "),
                           Element(ElementType::Tag, "%s - "),
                           Element(ElementType::Message)});
}

std::string MessageFormatter::format(const LogMessage& message) const {
  std::string formattedMessage;

  std::string tempStr;
  FormatBuffer curElementBuffer;
  for (const Element& element : m_elements) {
    const char* curElement;
    switch (element.type) {
      case ElementType::String:
        curElement = element.format.c_str();
        break;
      case ElementType::Level:
        assert(static_cast<std::size_t>(message.level) < m_levelNameMap.size());
        curElement = curElementBuffer.format(
            element.format.c_str(),
            m_levelNameMap[static_cast<std::size_t>(message.level)].c_str());
        break;
      case ElementType::Tag:
        if (!message.tag || !*message.tag) continue;

        curElement = curElementBuffer.format(
            element.format.c_str(),
            limitString(tempStr, message.tag, element.limit));
        break;
      case ElementType::Message:
        curElement = curElementBuffer.format(
            element.format.c_str(),
            limitString(tempStr, message.message, element.limit));
        break;
      case ElementType::File:
        curElement = curElementBuffer.format(
            element.format.c_str(),
            limitString(tempStr, message.file, element.limit));
        break;
      case ElementType::Line:
        curElement =
            curElementBuffer.format(element.format.c_str(), message.line);
        break;
      case ElementType::Function:
        curElement = curElementBuffer.format(
            element.format.c_str(),
            limitString(tempStr, message.function, element.limit));
        break;
      case ElementType::FullFunction:
        curElement = curElementBuffer.format(
            element.format.c_str(),
            limitString(tempStr, message.fullFunction, element.limit));
        break;
      case ElementType::Time:
        if (m_timezone == Timezone::Local) {
          curElement = curElementBuffer.formatLocalTime(message.time,
                                                        element.format.c_str());
        } else {
          curElement = curElementBuffer.formatUtcTime(message.time,
                                                      element.format.c_str());
        }
        break;
      case ElementType::TimeMs: {
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            message.time.time_since_epoch());
        unsigned int msOffset =
            static_cast<unsigned int>(totalMs.count() % 1000);
        curElement = curElementBuffer.format(element.format.c_str(), msOffset);
        break;
      }
      case ElementType::ThreadId:
        curElement =
            curElementBuffer.format(element.format.c_str(), message.threadId);
        break;
      default:
        continue;
    }

    formattedMessage += curElement;
  }

  return formattedMessage;
}

}  // namespace logging
}  // namespace olp
