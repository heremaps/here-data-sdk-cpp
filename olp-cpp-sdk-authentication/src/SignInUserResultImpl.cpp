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

#include "SignInUserResultImpl.h"

#include "Constants.h"
#include "olp/core/network/HttpStatusCode.h"

namespace {
constexpr auto kTermsReacceptanceToken = "termsReacceptanceToken";
constexpr auto kTermsUrls = "url";
constexpr auto kTermsOfServiceUrl = "tos";
constexpr auto kTermsOfServiceUrlJson = "tosJSON";
constexpr auto kPrivatePolicyUrl = "pp";
constexpr auto kPrivatePolicyUrlJson = "ppJSON";
}  // namespace

namespace olp {
namespace authentication {

SignInUserResultImpl::SignInUserResultImpl() noexcept
    : SignInUserResultImpl(network::HttpStatusCode::ServiceUnavailable,
                           Constants::ERROR_HTTP_SERVICE_UNAVAILABLE) {}

SignInUserResultImpl::SignInUserResultImpl(
    int status, std::string error,
    std::shared_ptr<rapidjson::Document> json_document) noexcept
    : SignInResultImpl(status, error, json_document) {
  if (BaseResult::IsValid()) {
    if (json_document->HasMember(kTermsReacceptanceToken))
      term_acceptance_token_ =
          (*json_document)[kTermsReacceptanceToken].GetString();
    if (json_document->HasMember(kTermsUrls)) {
      rapidjson::Document::Object obj =
          (*json_document)[kTermsUrls].GetObject();
      if (obj.HasMember(kTermsOfServiceUrl))
        terms_of_service_url_ = obj[kTermsOfServiceUrl].GetString();
      if (obj.HasMember(kTermsOfServiceUrlJson))
        terms_of_service_url_json_ = obj[kTermsOfServiceUrlJson].GetString();
      if (obj.HasMember(kPrivatePolicyUrl))
        private_policy_url_ = obj[kPrivatePolicyUrl].GetString();
      if (obj.HasMember(kPrivatePolicyUrlJson))
        private_policy_url_json_ = obj[kPrivatePolicyUrlJson].GetString();
    }
  }
}

SignInUserResultImpl::~SignInUserResultImpl() = default;

const std::string& SignInUserResultImpl::GetTermAcceptanceToken() const {
  return term_acceptance_token_;
}

const std::string& SignInUserResultImpl::GetTermsOfServiceUrl() const {
  return terms_of_service_url_;
}

const std::string& SignInUserResultImpl::GetTermsOfServiceUrlJson() const {
  return terms_of_service_url_json_;
}

const std::string& SignInUserResultImpl::GetPrivatePolicyUrl() const {
  return private_policy_url_;
}

const std::string& SignInUserResultImpl::GetPrivatePolicyUrlJson() const {
  return private_policy_url_json_;
}

}  // namespace authentication
}  // namespace olp
