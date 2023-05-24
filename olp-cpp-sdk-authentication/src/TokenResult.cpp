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

#include <olp/authentication/TokenResult.h>

namespace olp {
namespace authentication {
TokenResult::TokenResult(std::string access_token, time_t expiry_time)
    : access_token_(std::move(access_token)), expiry_time_(expiry_time) {
  const auto now = std::time(nullptr);
  expires_in_ =
      std::chrono::seconds(expiry_time_ > now ? (expiry_time_ - now) : 0);
}

TokenResult::TokenResult(std::string access_token,
                         std::chrono::seconds expires_in)
    : access_token_(std::move(access_token)), expires_in_(expires_in) {
  expiry_time_ = std::time(nullptr) + expires_in_.count();
}

const std::string& TokenResult::GetAccessToken() const { return access_token_; }

time_t TokenResult::GetExpiryTime() const { return expiry_time_; }

std::chrono::seconds TokenResult::GetExpiresIn() const { return expires_in_; }

}  // namespace authentication
}  // namespace olp
