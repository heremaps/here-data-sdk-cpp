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

#pragma once

#include <ctime>
#include <memory>
#include <string>

#include "AuthenticationApi.h"
#include "SignInResult.h"

namespace olp {
namespace authentication {
class SignInUserResultImpl;

/**
 * @brief The SignInUserResult class represents the response to a user sign-in
 * operation.
 *
 * If a HTTP status of 412 (PRECONDITION_FAILED) or 201 (CREATED) is returned,
 * the response will return terms of re-acceptance. This response may be
 * received on the first sign in, or any subsequent sign-in, for any user
 * authentication method, as long as the user has not accepted the latest terms
 * for their country.
 */
class AUTHENTICATION_API SignInUserResult : public SignInResult {
 public:
  /**
   * @brief Destructor
   */
  ~SignInUserResult() override;

  /**
   * @brief Constructor
   */
  SignInUserResult() noexcept;

  /**
   * @brief Terms acceptance token getter method.
   * @return String containing the token required for the 'terms acceptance API'
   * (only filled when status is 412 or 201).
   */
  const std::string& GetTermAcceptanceToken() const;

  /**
   * @brief Terms of service url getter method.
   * @return String containing the URL to the most recent terms of service,
   * appropriate for the user's country and language (only filled when status is
   * 412 or 201).
   */
  const std::string& GetTermsOfServiceUrl() const;

  /**
   * @brief Terms of service url json getter method.
   * @return String containing the URL to the most recent JSON version of terms
   * of service, appropriate for the user's country and language (only filled
   * when status is 412 or 201).
   */
  const std::string& GetTermsOfServiceUrlJson() const;

  /**
   * @brief Private policy url getter method.
   * @return String containing the URL to the most recent privacy policy,
   * appropriate for the user's country and language (only filled when status is
   * 412 or 201).
   */
  const std::string& GetPrivatePolicyUrl() const;

  /**
   * @brief Private policy url json getter method.
   * @return String containing the URL to the most recent JSON version of
   * privacy policy, appropriate for the user's country and language (only
   * filled when status is 412 or 201).
   */
  const std::string& GetPrivatePolicyUrlJson() const;

 private:
  friend class AuthenticationClient;

  SignInUserResult(std::shared_ptr<SignInUserResultImpl> impl);

 private:
  std::shared_ptr<SignInUserResultImpl> impl_;
};
}  // namespace authentication
}  // namespace olp
