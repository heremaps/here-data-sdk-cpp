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
#include <memory>
#include <string>

#include "AuthenticationApi.h"
#include "SignInResult.h"

namespace olp {
namespace authentication {
class SignInUserResultImpl;

/**
 * @brief A response to your sign-in operation.
 *
 * If the HTTP status of 412 (PRECONDITION_FAILED) or 201 (CREATED) is returned,
 * the response returns terms of re-acceptance. Not to receive this response
 * again, accept the latest terms for your country.
 */
class AUTHENTICATION_API SignInUserResult : public SignInResult {
 public:
  /**
   * @brief A default destructor.
   */
  ~SignInUserResult() override;

  /**
   * @brief A default constructor.
   */
  SignInUserResult() noexcept;

  /**
   * @brief Gets the terms acceptance token.
   *
   * @return The string that contains the token required for the terms
   * acceptance API (filled in only when the HTTP status is 412 or 201).
   */
  const std::string& GetTermAcceptanceToken() const;

  /**
   * @brief Gets the URL of the terms of service.
   *
   * @return The string that contains the URL of the most recent terms of
   * service appropriate for your country and language (filled in only when
   * the HTTP status is 412 or 201).
   */
  const std::string& GetTermsOfServiceUrl() const;

  /**
   * @brief Gets the URL of the most recent JSON version of the terms of
   * service.
   *
   * @return The string that contains the URL of the most recent JSON version of
   * the terms of service appropriate for your country and language (filled in
   * only when the HTTP status is 412 or 201).
   */
  const std::string& GetTermsOfServiceUrlJson() const;

  /**
   * @brief Gets the most recent privacy policy URL.
   *
   * @return The string that contains the URL of the most recent privacy policy
   * appropriate for your country and language (filled in only when the HTTP
   * status is 412 or 201).
   */
  const std::string& GetPrivatePolicyUrl() const;

  /**
   * @brief Gets the URL of the most recent privacy policy JSON.
   *
   * @return The string that contains the URL of the most recent JSON version of
   * the privacy policy appropriate for your country and language
   * (filled in only when the HTTP status is 412 or 201).
   */
  const std::string& GetPrivatePolicyUrlJson() const;

 private:
  friend class AuthenticationClientImpl;

  SignInUserResult(std::shared_ptr<SignInUserResultImpl> impl);

 private:
  std::shared_ptr<SignInUserResultImpl> impl_;
};
}  // namespace authentication
}  // namespace olp
