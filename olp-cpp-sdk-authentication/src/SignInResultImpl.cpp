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

#include "SignInResultImpl.h"

#include "Constants.h"
#include "olp/core/http/HttpStatusCode.h"

namespace {
constexpr auto kTokenType = "tokenType";
constexpr auto kUserId = "userId";
constexpr auto kScope = "scope";
}  // namespace

namespace olp {
namespace authentication {

SignInResultImpl::SignInResultImpl() noexcept
    : SignInResultImpl(http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       Constants::ERROR_HTTP_SERVICE_UNAVAILABLE) {}

SignInResultImpl::SignInResultImpl(
    int status, std::string error,
    std::shared_ptr<rapidjson::Document> json_document) noexcept
    : BaseResult(status, error, json_document), expiry_time_() {
  is_valid_ = this->BaseResult::IsValid() &&
              json_document->HasMember(Constants::ACCESS_TOKEN) &&
              json_document->HasMember(kTokenType) &&
              json_document->HasMember(Constants::EXPIRES_IN);

  // Extra response data if no errors reported
  if (!HasError()) {
    if (!IsValid()) {
      status_ = http::HttpStatusCode::SERVICE_UNAVAILABLE;
      error_.message = Constants::ERROR_HTTP_SERVICE_UNAVAILABLE;
    } else {
      if (json_document->HasMember(Constants::ACCESS_TOKEN))
        access_token_ = (*json_document)[Constants::ACCESS_TOKEN].GetString();
      if (json_document->HasMember(kTokenType))
        token_type_ = (*json_document)[kTokenType].GetString();
      if (json_document->HasMember(Constants::REFRESH_TOKEN))
        refresh_token_ = (*json_document)[Constants::REFRESH_TOKEN].GetString();
      if (json_document->HasMember(Constants::EXPIRES_IN)) {
        expiry_time_ = std::time(nullptr) +
                       (*json_document)[Constants::EXPIRES_IN].GetUint();
        expires_in_ = std::chrono::seconds(
            (*json_document)[Constants::EXPIRES_IN].GetUint());
      }
      if (json_document->HasMember(kUserId))
        user_identifier_ = (*json_document)[kUserId].GetString();
      if (json_document->HasMember(kScope))
        scope_ = (*json_document)[kScope].GetString();
    }
  }
}

SignInResultImpl::~SignInResultImpl() = default;

const std::string& SignInResultImpl::GetAccessToken() const {
  return access_token_;
}

const std::string& SignInResultImpl::GetTokenType() const {
  return token_type_;
}

const std::string& SignInResultImpl::GetRefreshToken() const {
  return refresh_token_;
}

std::chrono::seconds SignInResultImpl::GetExpiresIn() const {
  return expires_in_;
}

time_t SignInResultImpl::GetExpiryTime() const { return expiry_time_; }

const std::string& SignInResultImpl::GetUserIdentifier() const {
  return user_identifier_;
}

const std::string& SignInResultImpl::GetScope() const { return scope_; }

bool SignInResultImpl::IsValid() const { return is_valid_; }

}  // namespace authentication
}  // namespace olp
