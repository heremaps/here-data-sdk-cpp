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

#include <olp/core/utils/Url.h>

#include <stdint.h>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

namespace olp {
namespace utils {
// -------------------------------------------------------------------------------------------------

namespace {
const std::string space_type_chars = " \t\r\v\f\n";
const std::string path_separator = "/";

bool is_hex_digit(char c) {
  return std::isxdigit(static_cast<unsigned char>(c));
}

// -------------------------------------------------------------------------------------------------

unsigned int hex_to_uint(const char* hexStr) {
  unsigned int c;
  std::stringstream ss;
  ss << std::hex << hexStr;
  ss >> c;
  return c;
}

}  // Anonymous namespace

// -------------------------------------------------------------------------------------------------

std::string Url::Decode(const std::string& in) {
  unsigned int read_idx = 0;

  std::string out;
  out.reserve(in.size());

  while (read_idx < in.size()) {
    // unescape character
    if ((read_idx < in.size() - 2) && (in[read_idx] == '%') &&
        is_hex_digit(in[read_idx + 1]) && is_hex_digit(in[read_idx + 2])) {
      // convert character
      char hex_str[3];
      hex_str[0] = in[++read_idx];
      hex_str[1] = in[++read_idx];
      hex_str[2] = '\0';

      out.push_back(static_cast<char>(hex_to_uint(hex_str)));
    } else if (in[read_idx] == '+') {
      // replace + with space (application/x-www-form-urlencoded)
      out.push_back(' ');
    } else {
      out.push_back(in[read_idx]);
    }
    read_idx++;
  }
  return out;
}

// -------------------------------------------------------------------------------------------------

/**
 * See http://en.wikipedia.org/wiki/Percent-encoding for explanation
 */

std::string Url::Encode(const std::string& in) {
  // Clear output before appending it
  std::string out;
  out.reserve(in.size() * 3);

  const size_t HEX_BUF_SIZE = 20;
  char hex_str[HEX_BUF_SIZE] = {};

  for (size_t i = 0; i < in.size(); ++i) {
    char current_char = static_cast<char>(in[i]);
    if ((current_char < 'a' || current_char > 'z') &&
        (current_char < 'A' || current_char > 'Z') &&
        (current_char < '0' || current_char > '9') && current_char != '.' &&
        current_char != '-' && current_char != '~' && current_char != '_') {
      snprintf(hex_str, HEX_BUF_SIZE, "%%%.2X",
               static_cast<unsigned char>(current_char));
      out += hex_str;
    } else {
      out.push_back(current_char);
    }
  }

  return out;
}
// -------------------------------------------------------------------------------------------------

}  // namespace utils
}  // namespace olp
