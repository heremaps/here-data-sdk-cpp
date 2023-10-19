/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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

#include <olp/authentication/Crypto.h>

#include <algorithm>
#include <cstdint>

namespace olp {
namespace authentication {

namespace {

// SHA256 Algorithm from
// https://csrc.nist.gov/csrc/media/publications/fips/180/4/final/documents/fips180-4-draft-aug2014.pdf

#define SHA256_HASH_VALUE_LENGTH 8
#define SHA256_CONSTANTS_LENGTH 64
#define SHA256_LAST_CHUNK_LENGTH 64
#define SHA256_MESSAGE_SCHEDULE_LENGTH 64
#define SHA256_DIGEST_LENGTH 32

#define ROTR(x, n) ((x >> n) | (x << (32 - n)))
#define SHA256_CH(x, y, z) ((x & y) ^ (~x & z))
#define SHA256_MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define SHA256_SUM0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SHA256_SUM1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SHA256_SIGMA0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3))
#define SHA256_SIGMA1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10))

static uint32_t SHA256_K[SHA256_CONSTANTS_LENGTH] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

// HMAC Algorithm from
// https://csrc.nist.gov/csrc/media/publications/fips/198/1/final/documents/fips-198-1_final.pdf

#define HMAC_IPAD_BYTE 0x36
#define HMAC_OPAD_BYTE 0x5c
#define HMAC_B 64

std::vector<uint32_t> Sha256Init() {
  std::vector<uint32_t> hashValue;
  hashValue.push_back(0x6a09e667);
  hashValue.push_back(0xbb67ae85);
  hashValue.push_back(0x3c6ef372);
  hashValue.push_back(0xa54ff53a);
  hashValue.push_back(0x510e527f);
  hashValue.push_back(0x9b05688c);
  hashValue.push_back(0x1f83d9ab);
  hashValue.push_back(0x5be0cd19);

  return hashValue;
}

void Sha256Transform(const std::vector<unsigned char>& current_chunk,
                     unsigned long start_index,
                     std::vector<uint32_t>& hash_value) {
  std::array<uint32_t, SHA256_MESSAGE_SCHEDULE_LENGTH> w;

  for (int i = 0, j = 0; i < 16; i++, j += 4)
    w[i] = (current_chunk[start_index + j] << 24) |
           (current_chunk[start_index + j + 1] << 16) |
           (current_chunk[start_index + j + 2] << 8) |
           (current_chunk[start_index + j + 3]);
  for (int i = 16; i < SHA256_MESSAGE_SCHEDULE_LENGTH; i++) {
    w[i] = SHA256_SIGMA1(w[i - 2]) + w[i - 7] + SHA256_SIGMA0(w[i - 15]) +
           w[i - 16];
  }

  std::array<uint32_t, SHA256_HASH_VALUE_LENGTH> working_var;
  for (int i = 0; i < SHA256_HASH_VALUE_LENGTH; i++) {
    working_var[i] = hash_value[i];
  }

  for (int i = 0; i < SHA256_MESSAGE_SCHEDULE_LENGTH; i++) {
    uint32_t t1 = working_var[7] + SHA256_SUM1(working_var[4]) +
                  SHA256_CH(working_var[4], working_var[5], working_var[6]) +
                  SHA256_K[i] + w[i];
    uint32_t t2 = SHA256_SUM0(working_var[0]) +
                  SHA256_MAJ(working_var[0], working_var[1], working_var[2]);
    working_var[7] = working_var[6];
    working_var[6] = working_var[5];
    working_var[5] = working_var[4];
    working_var[4] = working_var[3] + t1;
    working_var[3] = working_var[2];
    working_var[2] = working_var[1];
    working_var[1] = working_var[0];
    working_var[0] = t1 + t2;
  }

  for (int i = 0; i < SHA256_HASH_VALUE_LENGTH; i++) {
    hash_value[i] += working_var[i];
  }
}

Crypto::Sha256Digest ComputeSha256(const std::vector<unsigned char>& src) {
  auto hash_value = Sha256Init();
  std::vector<std::vector<unsigned char> > chucks_to_process;

  const auto length = src.size();
  const auto chunks_count = length / SHA256_LAST_CHUNK_LENGTH;
  const auto last_chunk_size = length % SHA256_LAST_CHUNK_LENGTH;

  std::vector<unsigned char> last_chunk;
  if (last_chunk_size < 56) {
    last_chunk.assign(src.begin() + (chunks_count * SHA256_LAST_CHUNK_LENGTH),
                      src.end());
    last_chunk.push_back(0x80);
  } else {
    // Not enough empty space left in the last chunk, need another one
    std::vector<unsigned char> second_last_chunk;
    second_last_chunk.assign(
        src.begin() + (chunks_count * SHA256_LAST_CHUNK_LENGTH), src.end());
    second_last_chunk.push_back(0x80);
    auto chunkSize = second_last_chunk.size();
    for (auto i = chunkSize; i < SHA256_LAST_CHUNK_LENGTH; i++) {
      second_last_chunk.push_back(0);
    }
    chucks_to_process.push_back(second_last_chunk);
  }

  for (auto i = last_chunk.size(); i < (SHA256_LAST_CHUNK_LENGTH - 8); i++) {
    last_chunk.push_back(0);
  }
  uint64_t srcLength = length * 8;
  for (int i = 8; i > 0; --i) {
    last_chunk.push_back((srcLength >> ((i - 1) * 8)) & 0xff);
  }
  chucks_to_process.push_back(last_chunk);

  for (auto i = 0UL; i < chunks_count; i++) {
    Sha256Transform(src, i * SHA256_LAST_CHUNK_LENGTH, hash_value);
  }

  for (const auto& chunk : chucks_to_process) {
    Sha256Transform(chunk, 0, hash_value);
  }

  Crypto::Sha256Digest ret;
  for (int i = 0, j = 0; i < SHA256_HASH_VALUE_LENGTH; i++, j += 4) {
    uint32_t value = hash_value[i];
    auto v3 = (unsigned char)value;
    auto v2 = (unsigned char)(value >>= 8);
    auto v1 = (unsigned char)(value >>= 8);
    ret[j + 0] = (unsigned char)(value >>= 8);
    ret[j + 1] = v1;
    ret[j + 2] = v2;
    ret[j + 3] = v3;
  }

  return ret;
}

std::vector<unsigned char> ToUnsignedCharVector(const std::string& src) {
  std::vector<unsigned char> ret(src.length());
  std::transform(src.begin(), src.end(), ret.begin(),
                 [](char c) { return static_cast<unsigned char>(c); });
  return ret;
}

Crypto::Sha256Digest ComputeHmacSha256(const std::string& key,
                                       const std::string& message) {
  std::vector<unsigned char> k0;

  // Step 1 - 3
  auto key_length = key.length();
  std::vector<uint8_t> keyVec = ToUnsignedCharVector(key);
  if (key_length <= HMAC_B) {
    k0.assign(keyVec.begin(), keyVec.end());
  } else {
    auto new_key = ComputeSha256(keyVec);
    key_length = new_key.size();
    k0.assign(new_key.begin(), new_key.end());
  }

  for (auto i = key_length; i < HMAC_B; i++) {
    k0.push_back(0);
  }

  // Step 4 - 5
  std::vector<uint8_t> k0XORipad_msg;
  for (int i = 0; i < HMAC_B; i++) {
    k0XORipad_msg.push_back(k0[i] ^ HMAC_IPAD_BYTE);
  }

  std::vector<uint8_t> message_vec = ToUnsignedCharVector(message);
  k0XORipad_msg.insert(k0XORipad_msg.end(), message_vec.begin(),
                       message_vec.end());

  // Step 6
  auto hk0XORipad_msg = ComputeSha256(k0XORipad_msg);

  // Step 7 - 8
  std::vector<uint8_t> s8;
  for (int i = 0; i < HMAC_B; i++) {
    s8.push_back(k0[i] ^ HMAC_OPAD_BYTE);
  }

  s8.insert(s8.end(), hk0XORipad_msg.begin(), hk0XORipad_msg.end());

  // Step 9
  return ComputeSha256(s8);
}
}  // namespace

Crypto::Sha256Digest Crypto::Sha256(const std::vector<unsigned char>& content) {
  return ComputeSha256(content);
}

Crypto::Sha256Digest Crypto::HmacSha256(const std::string& key,
                                        const std::string& message) {
  return ComputeHmacSha256(key, message);
}
}  // namespace authentication
}  // namespace olp
