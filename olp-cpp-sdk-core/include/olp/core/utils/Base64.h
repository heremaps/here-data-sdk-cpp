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

#include <stdint.h>
#include <cstdint>
#include <string>
#include <vector>

#include <olp/core/CoreApi.h>

namespace olp {
namespace utils {

/**
 * @brief Encodes a binary stream into a Base64 text.
 *
 * @param bytes The data to be encoded.
 * @param url If set to true, the "=" padding will be omitted. The default value
 * is false.
 *
 * @return The Base64 encoded string.
 */
CORE_API std::string Base64Encode(const std::vector<uint8_t>& bytes,
                                  bool url = false);

/**
 * @brief Encodes a string into a Base64 text.
 *
 * @param bytes The data to be encoded.
 * @param url If set to true, the "=" padding will be omitted. The default value
 * is false.
 *
 * @return The Base64 encoded string.
 */
CORE_API std::string Base64Encode(const std::string& bytes, bool url = false);

/**
 * @brief Encodes a binary stream into a Base64 text.
 *
 * @param bytes The data to be encoded.
 * @param size The length of the byte array.
 * @param url If set to true, the "=" padding will be omitted. The default value
 * is false.
 *
 * @return The Base64 encoded string.
 */
CORE_API std::string Base64Encode(const void* bytes, size_t size,
                                  bool url = false);

/**
 * @brief Decodes a Base64 string into a binary stream.
 *
 * @param[in] string The Base64 string to be decoded.
 * @param[out] bytes The vector containing the decoded bytes.
 * @param[in] write_null_bytes True if the decoded NULL bytes should be written
 * to the output; false otherwise. The default value is true.
 *
 * @return True if the decoding was successful; false otherwise.
 */
CORE_API bool Base64Decode(const std::string& string,
                           std::vector<std::uint8_t>& bytes,
                           bool write_null_bytes = true);

}  // namespace utils
}  // namespace olp
