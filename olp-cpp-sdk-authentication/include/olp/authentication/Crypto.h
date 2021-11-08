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

#include <array>
#include <string>
#include <vector>

#include <olp/authentication/AuthenticationApi.h>

namespace olp {
namespace authentication {

/// The cryptographic algoritms used by the library.
class AUTHENTICATION_API Crypto {
 public:
  /// The hash length after the SHA-256 encryption.
  static const size_t Sha256DigestLength = 32;

  /// An alias for the hash.
  using Sha256Digest = std::array<unsigned char, Sha256DigestLength>;

  /**
   * @brief Sha256 cryptographic function used to compute the hash value for a
   * buffer.
   *
   * @param content A vector of unsigned chars for which the hash is computed.
   *
   * @return An array of 32 bytes that represent a hash value.
   */
  static Sha256Digest Sha256(const std::vector<unsigned char>& content);

  /**
   * @brief HMAC function based on SHA-256 hash function.
   *
   * @param key A key.
   * @param message A message.
   *
   * @return An array of 32 bytes that represent a hash value.
   */
  static Sha256Digest HmacSha256(const std::string& key,
                                 const std::string& message);
};

}  // namespace authentication
}  // namespace olp
