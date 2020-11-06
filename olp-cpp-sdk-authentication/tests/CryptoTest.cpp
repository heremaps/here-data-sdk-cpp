/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <gtest/gtest.h>

#include <sstream>

#include <olp/authentication/Crypto.h>

namespace {

using olp::authentication::Crypto;

std::string ToString(Crypto::Sha256Digest hash) {
  std::stringstream stream;
  for (auto element : hash) {
    stream << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int32_t>(element);
  }
  return stream.str();
}

TEST(CryptoTest, Sha256) {
  {
    SCOPED_TRACE("Sha256");
    const auto expectedHash =
        "d3f3165289cc4cfbf8b33efe78f90e2bd5133084ab8593f12c19f9a0cdaca597";

    const std::string content = "empty string";
    const std::vector<unsigned char> bytes(std::begin(content),
                                           std::end(content));
    const auto computedHash = Crypto::Sha256(bytes);
    const auto computedHashStr = ToString(computedHash);
    EXPECT_EQ(computedHashStr, expectedHash);
  }
}

TEST(CryptoTest, HMACSha256) {
  {
    SCOPED_TRACE("HMACSha256");
    const auto expectedHash =
        "f0a6ab128de9a764620902043941a6ef22e5426d9e06917525ea695f111ca139";

    const std::string content = "empty string";
    const auto computedHash = Crypto::HmacSha256("secret", content);
    const auto computedHashStr = ToString(computedHash);
    EXPECT_EQ(computedHashStr, expectedHash);
  }
}

}  // namespace
