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

#include "olp/core/utils/Url.h"

#include <stdint.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace olp {
namespace utils {

namespace {

bool IsHexDigit(char c) { return std::isxdigit(static_cast<unsigned char>(c)); }

unsigned int HexToInt(const char* hex) {
  unsigned int out;
  std::stringstream ss;
  ss << std::hex << hex;
  ss >> out;
  return out;
}

}  // namespace

std::string Url::Decode(const std::string& in) {
  unsigned int read_idx = 0;

  std::string out;
  out.reserve(in.size());

  while (read_idx < in.size()) {
    // unescape character
    if ((read_idx < in.size() - 2) && (in[read_idx] == '%') &&
        IsHexDigit(in[read_idx + 1]) && IsHexDigit(in[read_idx + 2])) {
      // convert character
      char hex_str[3];
      hex_str[0] = in[++read_idx];
      hex_str[1] = in[++read_idx];
      hex_str[2] = '\0';

      out.push_back(static_cast<char>(HexToInt(hex_str)));
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

std::string Url::Encode(const std::string& in) {
  // See http://en.wikipedia.org/wiki/Percent-encoding for explanation

  // Clear output before appending it
  std::string out;
  out.reserve(in.size() * 3);

  constexpr size_t kHexBufSize = 20;
  char hex_str[kHexBufSize] = {};

  for (size_t i = 0; i < in.size(); ++i) {
    char current_char = static_cast<char>(in[i]);
    if ((current_char < 'a' || current_char > 'z') &&
        (current_char < 'A' || current_char > 'Z') &&
        (current_char < '0' || current_char > '9') && current_char != '.' &&
        current_char != '-' && current_char != '~' && current_char != '_') {
      snprintf(hex_str, kHexBufSize, "%%%.2X",
               static_cast<unsigned char>(current_char));
      out += hex_str;
    } else {
      out.push_back(current_char);
    }
  }

  return out;
}

std::string Url::Construct(
    const std::string& base, const std::string& path,
    const std::multimap<std::string, std::string>& query_params) {
  std::ostringstream url_ss;
  url_ss << base << path;

  bool first_param = true;
  for (const auto& query_param : query_params) {
    if (first_param) {
      url_ss << "?";
      first_param = false;
    } else {
      url_ss << "&";
    }

    const auto& key = query_param.first;
    const auto& value = query_param.second;
    url_ss << olp::utils::Url::Encode(key) << "="
           << olp::utils::Url::Encode(value);
  }

  return url_ss.str();
}

porting::optional<Url::HostAndRest> Url::ParseHostAndRest(
    const std::string& url) {
  const auto host_and_rest_regex =
      R"(^(.*:\/\/[A-Za-z0-9\-\.]+(:[0-9]+)?)(.*)$)";
  std::regex regex{host_and_rest_regex};
  std::smatch match;
  if (!std::regex_search(url, match, regex) || match.size() != 4) {
    return porting::none;
  }

  const auto host = std::string{match[1]};
  const auto rest = std::string{match[3]};
  return std::make_pair(host, rest);
}

}  // namespace utils
}  // namespace olp
