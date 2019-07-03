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

using namespace rapidjson;

namespace olp {
namespace authentication {
static const char* TERMS_REACCEPTANCE_TOKEN = "termsReacceptanceToken";
static const char* TERMS_URLS = "url";
static const char* TERMS_OF_SERVICE_URL = "tos";
static const char* TERMS_OF_SERVICE_URL_JSON = "tosJSON";
static const char* PRIVATE_POLICY_URL = "pp";
static const char* PRIVATE_POLICY_URL_JSON = "ppJSON";

SignInUserResultImpl::SignInUserResultImpl(
    int status, std::string error, std::shared_ptr<Document> json_document)
    : SignInResultImpl(status, error, json_document) {
  if (BaseResult::IsValid()) {
    if (json_document->HasMember(TERMS_REACCEPTANCE_TOKEN))
      term_acceptance_token_ =
          (*json_document)[TERMS_REACCEPTANCE_TOKEN].GetString();
    if (json_document->HasMember(TERMS_URLS)) {
      Document::Object obj = (*json_document)[TERMS_URLS].GetObject();
      if (obj.HasMember(TERMS_OF_SERVICE_URL))
        terms_of_service_url_ = obj[TERMS_OF_SERVICE_URL].GetString();
      if (obj.HasMember(TERMS_OF_SERVICE_URL_JSON))
        terms_of_service_url_json_ = obj[TERMS_OF_SERVICE_URL_JSON].GetString();
      if (obj.HasMember(PRIVATE_POLICY_URL))
        private_policy_url_ = obj[PRIVATE_POLICY_URL].GetString();
      if (obj.HasMember(PRIVATE_POLICY_URL_JSON))
        private_policy_url_json_ = obj[PRIVATE_POLICY_URL_JSON].GetString();
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
