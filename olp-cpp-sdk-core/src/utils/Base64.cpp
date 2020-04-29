/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "olp/core/utils/Base64.h"

#include <algorithm>

// clang-format off
// Do not change this order as throw_exception.hpp defines
// boost::throw_exception needed by all other boost classes
#include <boost/throw_exception.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
// clang-format on

namespace olp {
namespace utils {

namespace {
bool NotBase64Symbol(char c) {
  return !((c >= '/' && c <= '9') || (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') || c == '+');
}

bool IsValidBase64(const std::string& string) {
  const size_t length = string.size();
  if (length == 0) {
    // empty string is a valid base64
    return true;
  }

  if (length % 4 != 0) {
    return false;
  }

  // check for padding (only last two characters)
  auto end_no_padding = string.end();
  if (string[length - 1] == '=') {
    end_no_padding = std::prev(end_no_padding);
    if (string[length - 2] == '=') {
      end_no_padding = std::prev(end_no_padding);
    }
  }

  return std::find_if(string.begin(), end_no_padding, NotBase64Symbol) ==
         end_no_padding;
}
}  // anonymous namespace

std::string Base64Encode(const void* data, size_t size) {
  using namespace boost::archive::iterators;
  using It = base64_from_binary<transform_width<const uint8_t*, 6, 8> >;

  if (size == 0 || !data) {
    return {};
  }

  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
  auto return_string = std::string(It(bytes), It(bytes + size));
  return_string.append((3 - size % 3) % 3, '=');

  return return_string;
}

std::string Base64Encode(const std::vector<uint8_t>& bytes) {
  return Base64Encode(reinterpret_cast<const void*>(bytes.data()),
                      bytes.size());
}

std::string Base64Encode(const std::string& bytes) {
  return Base64Encode(reinterpret_cast<const void*>(bytes.c_str()),
                      bytes.size());
}

bool Base64Decode(const std::string& string, std::vector<std::uint8_t>& bytes,
                  bool write_null_bytes) {
  using namespace boost::archive::iterators;
  using It =
      transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;

  if (string.empty()) {
    bytes.clear();
    return true;
  }

  // check for validity of the input as boost will throw an exception
  if (!IsValidBase64(string)) {
    return false;
  }

  const size_t length = string.size();
  // check for padding (only last two characters)
  // we have checked string.empty() and IsValidBase64(), that means string has
  // at least 4 bytes
  std::string::const_iterator end_no_padding = string.end();
  if (string[length - 1] == '=') {
    end_no_padding = std::prev(end_no_padding);
    if (string[length - 2] == '=') {
      end_no_padding = std::prev(end_no_padding);
    }
  }

  bytes.resize(length / 4 * 3);
  auto it =
      write_null_bytes
          ? std::copy(It(std::begin(string)), It(end_no_padding), bytes.begin())
          : std::copy_if(It(std::begin(string)), It(end_no_padding),
                         bytes.begin(), [](char c) { return c != '\0'; });
  bytes.resize(std::distance(bytes.begin(), it));

  return true;
}

}  // namespace utils
}  // namespace olp
