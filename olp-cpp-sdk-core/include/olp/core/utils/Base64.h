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
 * @brief Encodes binary stream into base64 text.
 *
 * @param bytes Data to be encoded.
 *
 * @return Base64 encoded string.
 */
CORE_API std::string Base64Encode(const std::vector<std::uint8_t>& bytes);

/**
 * @brief Encodes binary stream into base64 text.
 *
 * @param bytes Data to be encoded.
 *
 * @return Base64 encoded string.
 */
CORE_API std::string Base64Encode(const std::string& bytes);

/**
 * @brief Encodes binary stream into base64 text.
 *
 * @param bytes Data to be encoded.
 * @param size The length of the bytes array.
 *
 * @return Base64 encoded string.
 */
CORE_API std::string Base64Encode(const void* bytes, size_t size);

/**
 * @brief Decodes base64 into a binary stream.
 *
 * @param[in] string Base64 string to be decoded.
 * @param[out] bytes Vector containing decoded bytes.
 * @param[in] write_null_bytes True if decoded null bytes should be written to
 * the output, false otherwise. Default is true.
 *
 * @return \c true if decoding was successful, \c false otherwise.
 */
CORE_API bool Base64Decode(const std::string& string,
                           std::vector<std::uint8_t>& bytes,
                           bool write_null_bytes = true);

}  // namespace utils
}  // namespace olp
