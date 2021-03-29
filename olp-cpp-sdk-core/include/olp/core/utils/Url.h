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

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/CoreApi.h>

namespace olp {
namespace utils {

/**
 * @brief Builds and parses URLs.
 */
class CORE_API Url {
 public:
  /**
   * @brief Decodes a URL on a given input string by replacing
   * percent-encoded characters with actual ones.
   *
   * @param in The URL-encoded string.
   *
   * @return The URL-decoded result string.
   */
  static std::string Decode(const std::string& in);

  /**
   * @brief Encodes a given input string by escaping non-ASCII characters.
   *
   * @param in The URL string to be encoded.
   *
   * @return The URL-encoded result string.
   */
  static std::string Encode(const std::string& in);

  /**
   * @brief Produces a full URL from a URL base, path, and query parameters.
   *
   * @param base The base of the URL.
   * @param path The path part of the URL.
   * @param query_params The multimap of query parameters.
   *
   * @return The URL-encoded result string.
   */
  static std::string Construct(
      const std::string& base, const std::string& path,
      const std::multimap<std::string, std::string>& query_params);
};

}  // namespace utils
}  // namespace olp
