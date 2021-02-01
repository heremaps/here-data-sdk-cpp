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

#include <stdlib.h>
#include <string>

namespace olp {
namespace client {

/// Simple tokenizer for char separated strings
struct Tokenizer {
  Tokenizer(const std::string& input, char separator)
      : str_(input), pos_(0), sep_(separator) {}

  bool HasNext() const { return !str_.empty() && (pos_ != std::string::npos); }

  std::string Next() {
    if (pos_ == std::string::npos) {
      return {};
    }

    size_t begin = pos_;
    size_t separator_position = str_.find_first_of(sep_, pos_);

    // handle last token in string
    if (separator_position == std::string::npos) {
      pos_ = std::string::npos;
      return str_.substr(begin);
    }

    pos_ = separator_position + 1;
    return str_.substr(begin, separator_position - begin);
  }

  // returns the entire remaining string
  std::string Tail() {
    if (pos_ == std::string::npos) {
      return {};
    }
    size_t begin = pos_;
    pos_ = std::string::npos;
    return str_.substr(begin);
  }

  Tokenizer& operator=(const Tokenizer&) = delete;

  const std::string& str_;
  size_t pos_{0};
  char sep_{'\0'};
};

}  // namespace client
}  // namespace olp
