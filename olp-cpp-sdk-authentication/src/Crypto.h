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

#include <string>
#include <vector>

namespace olp {
namespace authentication {
class Crypto {
 public:
  static std::vector<unsigned char> hmac_sha256(const std::string& key,
                                                const std::string& message);

 private:
  static std::vector<uint32_t> sha256Init();
  static std::vector<unsigned char> sha256(
      const std::vector<unsigned char>& src);
  static void sha256Transform(const std::vector<unsigned char>& currentChunk,
                              unsigned long startIndex,
                              std::vector<uint32_t>& hashValue);
  static std::vector<unsigned char> toUnsignedCharVector(
      const std::string& src);
};
}  // namespace authentication
}  // namespace olp
