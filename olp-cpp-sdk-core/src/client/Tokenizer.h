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

#include <stdlib.h>
#include <string>

namespace olp {
namespace client {
// simple tokenizer for char separated strings
struct Tokenizer {
  Tokenizer(const std::string& input, char separator)
      : str(input), pos(0), sep(separator) {}

  bool hasNext() const { return !str.empty() && (pos != std::string::npos); }

  std::string next() {
    if (pos == std::string::npos) return std::string();

    size_t begin = pos;
    size_t separator_position = str.find_first_of(sep, pos);

    // handle last token in string
    if (separator_position == std::string::npos) {
      pos = std::string::npos;
      return str.substr(begin);
    }

    pos = separator_position + 1;
    return str.substr(begin, separator_position - begin);
  }

  // returns the entire remaining string
  std::string tail() {
    if (pos == std::string::npos) return std::string();
    size_t begin = pos;
    pos = std::string::npos;
    return str.substr(begin);
  }

  std::uint64_t nextUInt64() {
    const std::string next_str = next();
    return strtoull(next_str.c_str(), nullptr, 10);
  }

  int nextInt() {
    const std::string next_str = next();
    return int(strtol(next_str.c_str(), nullptr, 10));
  }

  Tokenizer& operator=(const Tokenizer&) = delete;

  const std::string& str;
  size_t pos{0};
  char sep{'\0'};
};
}  // namespace client
}  // namespace olp
