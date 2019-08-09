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

#include "NetworkUtils.h"

#include <string>

namespace olp {
namespace network {
char NetworkUtils::SimpleToUpper(char c) {
  return ((c >= 'a') && (c <= 'z')) ? (c - 'a' + 'A') : c;
}

bool NetworkUtils::CaseInsensitiveCompare(const std::string& str1,
                                          const std::string& str2,
                                          size_t offset) {
  if ((str1.length() - offset) != str2.length()) return false;
  for (std::size_t i = 0; i < str2.length(); i++) {
    if (SimpleToUpper(str1[i + offset]) != SimpleToUpper(str2[i])) return false;
  }
  return true;
}

bool NetworkUtils::CaseInsensitiveStartsWith(const std::string& str1,
                                             const std::string& str2,
                                             size_t offset) {
  if ((str1.length() - offset) < str2.length()) return false;
  for (std::size_t i = 0; i < str2.length(); i++) {
    if (SimpleToUpper(str1[i + offset]) != SimpleToUpper(str2[i])) return false;
  }
  return true;
}

size_t NetworkUtils::CaseInsensitiveFind(const std::string& str1,
                                         const std::string& str2,
                                         size_t offset) {
  while (offset < str1.length() - str2.length()) {
    if (SimpleToUpper(str1[offset]) == SimpleToUpper(str2[0])) {
      if (CaseInsensitiveStartsWith(str1, str2, offset)) return offset;
    }
    offset++;
  }
  return std::string::npos;
}

}  // namespace network
}  // namespace olp
