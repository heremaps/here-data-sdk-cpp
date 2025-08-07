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

#include <olp/core/logging/FilterGroup.h>
#include <olp/core/porting/platform.h>
#include <string.h>
#include <array>
#include <fstream>
#include <string>
#include <utility>

namespace olp {
namespace logging {
// NOTE: Avoid boost::algorithm string functions because they cause linker
// errors when exceptions are disabled.

static std::pair<std::string, std::string> split(const std::string& str,
                                                 char c) {
  std::size_t index = str.find_first_of(c);
  if (index == std::string::npos)
    return std::make_pair(str, std::string());
  else
    return std::make_pair(str.substr(0, index), str.substr(index + 1));
}

static std::string trim(const std::string& str) {
  const char* whitespace = " \n\r\t\f\v";
  std::size_t first = str.find_first_not_of(whitespace);

  // All white space.
  if (first == std::string::npos) return std::string();

  std::size_t last = str.find_last_not_of(whitespace);
  return str.substr(first, last - first + 1);
}

static bool equalsIgnoreCase(const std::string& left,
                             const std::string& right) {
#ifdef PORTING_PLATFORM_WINDOWS
  return stricmp(left.c_str(), right.c_str()) == 0;
#else
  return strcasecmp(left.c_str(), right.c_str()) == 0;
#endif
}

porting::optional<Level> FilterGroup::stringToLevel(const std::string& levelStr) {
  if (equalsIgnoreCase(levelStr, "trace"))
    return Level::Trace;
  else if (equalsIgnoreCase(levelStr, "debug"))
    return Level::Debug;
  else if (equalsIgnoreCase(levelStr, "info"))
    return Level::Info;
  else if (equalsIgnoreCase(levelStr, "warning"))
    return Level::Warning;
  else if (equalsIgnoreCase(levelStr, "error"))
    return Level::Error;
  else if (equalsIgnoreCase(levelStr, "fatal"))
    return Level::Fatal;
  else if (equalsIgnoreCase(levelStr, "off"))
    return Level::Off;
  else
    return porting::none;
}

bool FilterGroup::load(const std::string& fileName) {
  std::ifstream stream(fileName);
  if (!stream) {
    clear();
    return false;
  }
  return load(stream);
}

bool FilterGroup::load(std::istream& stream) {
  clear();
  const unsigned int bufferSize = 1024;
  std::array<char, bufferSize> buffer;
  while (stream.getline(buffer.data(), buffer.size())) {
    std::string line = trim(buffer.data());
    if (line.empty() || line[0] == '#') continue;

    std::pair<std::string, std::string> splitLine = split(line, ':');
    std::string tag = trim(splitLine.first);
    std::string levelStr = trim(splitLine.second);

    porting::optional<Level> level = stringToLevel(levelStr);
    if (!level) return false;

    if (tag.empty())
      m_defaultLevel = *level;
    else
      m_tagLevels[tag] = *level;
  }

  return true;
}

}  // namespace logging
}  // namespace olp
