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

#include <olp/core/utils/Base64.h>

#include <boost/throw_exception.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <algorithm>

namespace olp {
namespace utils {
namespace {
bool notBase64Symbol(char c) {
  return !((c >= '/' && c <= '9') || (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') || c == '+');
}

bool isValidBase64(const std::string& s) {
  const size_t len = s.size();

  if (len == 0) {
    // empty string is a valid base64
    return true;
  }

  if (len % 4 != 0) {
    return false;
  }

  // check for padding (only last two characters)
  std::string::const_iterator endNoPadding = s.end();
  if (s[len - 1] == '=') {
    endNoPadding = std::prev(endNoPadding);
    if (s[len - 2] == '=') {
      endNoPadding = std::prev(endNoPadding);
    }
  }

  std::string::const_iterator notBase64 =
      std::find_if(s.begin(), endNoPadding, notBase64Symbol);

  return notBase64 == endNoPadding;
}

}  // anonymous namespace

std::string Base64Encode(const void* data, size_t size) {
  if (size == 0 || !data) {
    return std::string();
  }

  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);

  using namespace boost::archive::iterators;
  using It = base64_from_binary<transform_width<const uint8_t*, 6, 8> >;
  auto tmp = std::string(It(bytes), It(bytes + size));
  tmp.append((3 - size % 3) % 3, '=');
  return tmp;
}

std::string Base64Encode(const std::vector<uint8_t>& bytes) {
  return Base64Encode(reinterpret_cast<const void*>(bytes.data()),
                      bytes.size());
}

std::string Base64Encode(const std::string& bytes) {
  return Base64Encode(reinterpret_cast<const void*>(bytes.c_str()),
                      bytes.size());
}

bool Base64Decode(const std::string& s, std::vector<std::uint8_t>& bytes,
                  bool write_null_bytes) {
  if (s.empty()) {
    bytes.clear();
    return true;
  }

  // check for validity of the input as boost will throw an exception
  if (!isValidBase64(s)) {
    return false;
  }

  using namespace boost::archive::iterators;
  using It =
      transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;

  const size_t len = s.size();
  // check for padding (only last two characters)
  // we have checked s.empty() and isValidBase64(), that means len > 0 && len %
  // 4 == 0 s has at least 4 bytes
  std::string::const_iterator endNoPadding = s.end();
  if (s[len - 1] == '=') {
    endNoPadding = std::prev(endNoPadding);
    if (s[len - 2] == '=') {
      endNoPadding = std::prev(endNoPadding);
    }
  }

  bytes.resize(len / 4 * 3);
  auto it = write_null_bytes
                ? std::copy(It(std::begin(s)), It(endNoPadding), bytes.begin())
                : std::copy_if(It(std::begin(s)), It(endNoPadding),
                               bytes.begin(), [](char c) { return c != '\0'; });
  bytes.resize(std::distance(bytes.begin(), it));

  return true;
}

}  // namespace utils
}  // namespace olp
