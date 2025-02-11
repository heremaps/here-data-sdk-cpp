/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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
#include "olp/core/http/HttpStatusCode.h"

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
    : SignInUserResultImpl(http::HttpStatusCode::SERVICE_UNAVAILABLE,
                           Constants::ERROR_HTTP_SERVICE_UNAVAILABLE) {}

SignInUserResultImpl::SignInUserResultImpl(
    int status, std::string error,
    std::shared_ptr<boost::json::object> json_document) noexcept
    : SignInResultImpl(status, std::move(error), json_document) {
  if (BaseResult::IsValid()) {
    if (json_document->contains(kTermsReacceptanceToken))
      term_acceptance_token_ =
          (*json_document)[kTermsReacceptanceToken].get_string().c_str();
    if (json_document->contains(kTermsUrls)) {
      auto& obj = (*json_document)[kTermsUrls].get_object();
      if (obj.contains(kTermsOfServiceUrl))
        terms_of_service_url_ = obj[kTermsOfServiceUrl].get_string().c_str();
      if (obj.contains(kTermsOfServiceUrlJson))
        terms_of_service_url_json_ =
            obj[kTermsOfServiceUrlJson].get_string().c_str();
      if (obj.contains(kPrivatePolicyUrl))
        private_policy_url_ = obj[kPrivatePolicyUrl].get_string().c_str();
      if (obj.contains(kPrivatePolicyUrlJson))
        private_policy_url_json_ =
            obj[kPrivatePolicyUrlJson].get_string().c_str();
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
