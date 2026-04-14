/*
 * Copyright (C) 2019-2026 HERE Europe B.V.
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

#include <utility>

#include "Constants.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace authentication {

namespace {
constexpr auto kTokenType = "tokenType";
constexpr auto kUserId = "userId";
constexpr auto kScope = "scope";
constexpr auto kTokenTypeSnakeCase = "token_type";
constexpr auto kAccessTokenSnakeCase = "access_token";
constexpr auto kExpiresInSnakeCase = "expires_in";

bool HasAccessToken(const boost::json::object& document) {
  return document.contains(Constants::ACCESS_TOKEN) ||
         document.contains(kAccessTokenSnakeCase);
}

std::string ParseAccessToken(const boost::json::object& document) {
  if (document.contains(Constants::ACCESS_TOKEN)) {
    return document.at(Constants::ACCESS_TOKEN).get_string().c_str();
  }
  return document.at(kAccessTokenSnakeCase).get_string().c_str();
}

bool HasExpiresIn(const boost::json::object& document) {
  return document.contains(Constants::EXPIRES_IN) ||
         document.contains(kExpiresInSnakeCase);
}

uint64_t ParseExpiresIn(const boost::json::object& document) {
  if (document.contains(Constants::EXPIRES_IN)) {
    return document.at(Constants::EXPIRES_IN).to_number<uint64_t>();
  }
  return document.at(kExpiresInSnakeCase).to_number<uint64_t>();
}

bool HasTokenType(const boost::json::object& document) {
  return document.contains(kTokenType) ||
         document.contains(kTokenTypeSnakeCase);
}

std::string ParseTokenType(const boost::json::object& document) {
  if (document.contains(kTokenType)) {
    return document.at(kTokenType).get_string().c_str();
  }
  return document.at(kTokenTypeSnakeCase).get_string().c_str();
}

bool IsDocumentValid(const boost::json::object& document) {
  return HasAccessToken(document) && HasExpiresIn(document) &&
         HasTokenType(document);
}

}  // namespace

SignInResultImpl::SignInResultImpl() noexcept
    : SignInResultImpl(http::HttpStatusCode::SERVICE_UNAVAILABLE,
                       Constants::ERROR_HTTP_SERVICE_UNAVAILABLE) {}

SignInResultImpl::SignInResultImpl(
    int status, std::string error,
    std::shared_ptr<boost::json::object> json_document) noexcept
    : BaseResult(status, std::move(error), json_document),
      expiry_time_(),
      expires_in_() {
  is_valid_ = this->BaseResult::IsValid() && IsDocumentValid(*json_document);

  // Extra response data if no errors reported
  if (!HasError()) {
    if (!IsValid()) {
      status_ = http::HttpStatusCode::SERVICE_UNAVAILABLE;
      error_.message = Constants::ERROR_HTTP_SERVICE_UNAVAILABLE;
    } else {
      if (HasAccessToken(*json_document))
        access_token_ = ParseAccessToken(*json_document);
      if (HasTokenType(*json_document))
        token_type_ = ParseTokenType(*json_document);
      if (json_document->contains(Constants::REFRESH_TOKEN))
        refresh_token_ =
            json_document->at(Constants::REFRESH_TOKEN).get_string().c_str();
      if (HasExpiresIn(*json_document)) {
        const auto expires_in = ParseExpiresIn(*json_document);
        expiry_time_ = std::time(nullptr) + expires_in;
        expires_in_ = std::chrono::seconds(expires_in);
      }
      if (json_document->contains(kUserId))
        user_identifier_ = json_document->at(kUserId).get_string().c_str();
      if (json_document->contains(kScope))
        scope_ = json_document->at(kScope).get_string().c_str();
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
