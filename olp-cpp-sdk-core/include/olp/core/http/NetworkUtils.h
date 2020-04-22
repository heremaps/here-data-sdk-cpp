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

#pragma once

#include <string>

#include <olp/core/http/NetworkTypes.h>

namespace olp {
namespace http {
/**
 * @brief Network internal utilities.
 */
class CORE_API NetworkUtils {
 public:
  /**
   * @brief Changes the specified character to uppercase.
   *
   * If the character is already uppercase or non-alphabetical, it is
   * not changed.
   *
   * @param c The character that should be changed.
   *
   * @return The uppercase character.
   */
  static char SimpleToUpper(char c);
  /**
   * @brief Checks whether the source string matches the specified string
   * disregarding the case.
   *
   * @param str1 The source string.
   * @param str2 The string to which the source string is compared.
   * @param offset The offset for the source string from which the comparison
   * should start. Defaults to 0.
   *
   * @return True if the source string matches the specified string;
   * false otherwise.
   */
  static bool CaseInsensitiveCompare(const std::string& str1,
                                     const std::string& str2,
                                     size_t offset = 0);
  /**
   * @brief Checks whether the source string begins with the characters of
   * the specified string ignoring the case.
   *
   * @param str1 The source string.
   * @param str2 The string to which the source string is compared.
   * @param offset The offset for the source string from which the comparison
   * should start. Defaults to 0.
   *
   * @return True if the source string starts with the specified value; false
   * otherwise.
   */
  static bool CaseInsensitiveStartsWith(const std::string& str1,
                                        const std::string& str2,
                                        size_t offset = 0);
  /**
   * @brief Checks whether the source string contains characters of
   * the specified string ignoring the case.
   *
   * @param str1 The source string.
   * @param str2 The string to which the source string is compared.
   * @param offset The offset for the source string from which the comparison
   * should start. Defaults to 0.
   *
   * @return True if the source string contains characters of
   * the specified string ignoring the case; false otherwise.
   */
  static size_t CaseInsensitiveFind(const std::string& str1,
                                    const std::string& str2, size_t offset = 0);

  /**
   * @brief Extracts the user agent from the headers.
   *
   * The user agent is removed from the headers.
   *
   * @param headers The input headers.
   *
   * @return The user agent or an empty string if there is no user agent.
   */
  static std::string ExtractUserAgent(Headers& headers);
};  // The `NetworkUtils` class.

std::string HttpErrorToString(int error);

}  // namespace http
}  // namespace olp
