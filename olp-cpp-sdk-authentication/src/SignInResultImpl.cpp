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

#include <vector>

#include "Constants.h"
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/logging/Log.h"
#include "olp/core/utils/Base64.h"

namespace {
constexpr auto kTokenType = "tokenType";
constexpr auto kUserId = "userId";
constexpr auto kScope = "scope";
constexpr auto kLogTag = "SignIn";

}  // namespace

namespace olp {
namespace authentication {

namespace {
bool IsJwtToken(const std::string& token) { return token.front() == 'e'; }

bool IsHnToken(const std::string& token) { return token.front() == 'h'; }
}  // namespace

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
      if (json_document->HasMember(Constants::ACCESS_TOKEN)) {
        access_token_ = (*json_document)[Constants::ACCESS_TOKEN].GetString();
        client_id_ = ParseClientIdFromToken(access_token_.c_str());
      }
      if (json_document->HasMember(kTokenType))
        token_type_ = (*json_document)[kTokenType].GetString();
      if (json_document->HasMember(Constants::REFRESH_TOKEN))
        refresh_token_ = (*json_document)[Constants::REFRESH_TOKEN].GetString();
      if (json_document->HasMember(Constants::EXPIRES_IN))
        expiry_time_ = std::time(nullptr) +
                       (*json_document)[Constants::EXPIRES_IN].GetUint();
      expires_in_ = std::chrono::seconds(
          (*json_document)[Constants::EXPIRES_IN].GetUint());
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

const std::string& SignInResultImpl::GetClientId() const { return client_id_; }

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

std::string SignInResultImpl::ParseJwtToken(const std::string& token) {
  const std::string::size_type first_dot = token.find_first_of('.');
  if (first_dot == std::string::npos) {
    OLP_SDK_LOG_ERROR(kLogTag, "Cannot parse ClientId. Wrong token format.");
    return std::string();
  }

  const std::string jws_header_encoded = token.substr(0, first_dot);

  std::vector<std::uint8_t> decoded_bytes;
  if (!utils::Base64Decode(jws_header_encoded, decoded_bytes)) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Cannot parse ClientId. Non-decodable token format");
    return std::string();
  }

  std::string jws_header_decoded;
  std::copy(decoded_bytes.begin(), decoded_bytes.end(),
            std::back_inserter(jws_header_decoded));

  rapidjson::Document doc;
  doc.Parse(jws_header_decoded.c_str());

  if (doc.HasParseError() || !doc.IsObject()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Cannot parse ClientId. Defective token format");
    return std::string();
  }

  auto client_id_field = doc.FindMember("aid");
  if (client_id_field == doc.MemberEnd() ||
      !client_id_field->value.IsString()) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Cannot parse ClientId. Field does not exist or is not "
                      "a string json value");
    return std::string();
  }

  const std::string result = client_id_field->value.GetString();

  if (result.empty()) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Cannot parse ClientId. Incomplete token format");
  }

  return result;
}

std::string SignInResultImpl::ParseClientIdFromToken(const std::string& token) {
  if (token.empty()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Token is empty");
    return std::string();
  }

  if (IsJwtToken(token)) {
    return ParseJwtToken(token);
  } else if (IsHnToken(token)) {
    OLP_SDK_LOG_ERROR(kLogTag, "hN Tokens are not supported!");
    return std::string();
  } else {
    OLP_SDK_LOG_ERROR(kLogTag, "Unknown token format");
    return std::string();
  }
}

}  // namespace authentication
}  // namespace olp
