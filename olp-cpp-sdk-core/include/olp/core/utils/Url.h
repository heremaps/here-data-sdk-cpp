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
 * Class used for building and parsing Urls.
 */
class CORE_API Url {
 public:
  /**
   * @brief Performs URL decoding on a given input string by replacing
   * percent-encoded characters with the actual ones.
   *
   * @param in URL encoded string.
   *
   * @return URL decoded result string.
   */
  static std::string Decode(const std::string& in);

  /**
   * @brief Encodes a given input string by escaping non-ASCII characters.
   *
   * @param in URL string to be encoded.
   *
   * @return URL encoded result string.
   */
  static std::string Encode(const std::string& in);

  /**
   * @brief Produces full URL from url base, path and query parameters.
   *
   * @param base Base of the URL.
   * @param path Path part of the URL.
   * @param query_params Multipam of query parameters.
   *
   * @return URL encoded result string.
   */
  static std::string Construct(
      const std::string& base, const std::string& path,
      const std::multimap<std::string, std::string>& query_params);
};

}  // namespace utils
}  // namespace olp
