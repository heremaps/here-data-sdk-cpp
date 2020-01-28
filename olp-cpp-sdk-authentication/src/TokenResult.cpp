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

#include <olp/authentication/TokenResult.h>

namespace {
const std::string kOauth2TokenEndpoint = "/oauth2/token";
}  // namespace

namespace olp {
namespace authentication {
TokenResult::TokenResult(std::string access_token, time_t expiry_time,
                         int http_status, ErrorResponse error)
    : access_token_(std::move(access_token)),
      expiry_time_(expiry_time),
      http_status_(http_status),
      error_(std::move(error)) {
  expires_in_ = std::chrono::seconds(expiry_time_ - std::time(nullptr));
}

TokenResult::TokenResult(std::string access_token,
                         std::chrono::seconds expires_in, int http_status,
                         ErrorResponse error)
    : access_token_(std::move(access_token)),
      expires_in_(std::move(expires_in)),
      http_status_(http_status),
      error_(std::move(error)) {
  expiry_time_ = std::time(nullptr) + expires_in_.count();
}

TokenResult::TokenResult(const TokenResult& other) {
  this->access_token_ = other.access_token_;
  this->expiry_time_ = other.expiry_time_;
  this->http_status_ = other.http_status_;
  this->error_ = other.error_;
}

const std::string& TokenResult::GetAccessToken() const { return access_token_; }

time_t TokenResult::GetExpiryTime() const { return expiry_time_; }

std::chrono::seconds TokenResult::GetExpiresIn() const { return expires_in_; }

int TokenResult::GetHttpStatus() const { return http_status_; }

ErrorResponse TokenResult::GetErrorResponse() const { return error_; }

}  // namespace authentication
}  // namespace olp
