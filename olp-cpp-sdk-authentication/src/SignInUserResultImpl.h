/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#pragma once

#include <ctime>
#include <string>

#include "SignInResultImpl.h"

namespace olp {
namespace authentication {
/**
 * @brief The SignInUserResultImpl is the response class returned from a User
 * sign-in request.
 *
 * If a HTTP status of 412 (PRECONDITION_FAILED) is returned, the response will
 * return terms of re-acceptance. This response may be received on the first
 * sign in, or any subsequent sign-in, for any authentication method, as long as
 * the user has not accepted the latest terms for their country.
 */
class SignInUserResultImpl : public SignInResultImpl {
 public:
  SignInUserResultImpl() noexcept;

  SignInUserResultImpl(
      int status, std::string error,
      std::shared_ptr<rapidjson::Document> json_document = nullptr) noexcept;

  ~SignInUserResultImpl() override;

  /**
   * @brief Token required for the terms acceptance API
   * @return string containing the token required for the 'terms acceptance API'
   * (only filled when status is 412).
   */
  const std::string& GetTermAcceptanceToken() const;

  /**
   * @brief terms_of_service_url
   * @return URL to the most recent terms of service, appropriate for the user's
   * country and language (only filled when status is 412).
   */
  const std::string& GetTermsOfServiceUrl() const;

  /**
   * @brief terms_of_service_url_json
   * @return URL to the most recent JSON version of terms of service,
   * appropriate for the user's country and language (only filled when status is
   * 412).
   */
  const std::string& GetTermsOfServiceUrlJson() const;

  /**
   * @brief private_policy_url
   * @return URL to the most recent privacy policy, appropriate for the user's
   * country and language (only filled when status is 412).
   */
  const std::string& GetPrivatePolicyUrl() const;

  /**
   * @brief private_policy_url_json
   * @return URL to the most recent JSON version of privacy policy, appropriate
   * for the user's country and language (only filled when status is 412).
   */
  const std::string& GetPrivatePolicyUrlJson() const;

 private:
  std::string term_acceptance_token_;
  std::string terms_of_service_url_;
  std::string terms_of_service_url_json_;
  std::string private_policy_url_;
  std::string private_policy_url_json_;
};
}  // namespace authentication
}  // namespace olp
