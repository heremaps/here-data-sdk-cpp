/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <olp/core/client/OauthToken.h>

namespace olp {
namespace client {
OauthToken::OauthToken(std::string access_token, time_t expiry_time)
    : access_token_(std::move(access_token)), expiry_time_(expiry_time) {
  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  expires_in_ =
      std::chrono::seconds(expiry_time_ > now ? (expiry_time_ - now) : 0);
}

OauthToken::OauthToken(std::string access_token,
                       std::chrono::seconds expires_in)
    : access_token_(std::move(access_token)),
      expires_in_(std::move(expires_in)),
      expiry_time_(std::chrono::system_clock::to_time_t(
                       std::chrono::system_clock::now()) +
                   expires_in_.count()) {}

const std::string& OauthToken::GetAccessToken() const { return access_token_; }

time_t OauthToken::GetExpiryTime() const { return expiry_time_; }

std::chrono::seconds OauthToken::GetExpiresIn() const { return expires_in_; }

}  // namespace client
}  // namespace olp
