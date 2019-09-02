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
#include "ErrorResponse.h"

namespace olp {
namespace authentication {
class SignInResultImpl;

/**
 * @brief SignInResponse represents the response to a client or user sign-in
 * operation.
 */
class AUTHENTICATION_API SignInResult {
 public:
  /**
   * @brief Destructor
   */
  virtual ~SignInResult();

  /**
   * @brief Constructor
   */
  SignInResult() noexcept;

  /**
   * @brief HTTP status code getter method.
   * @return Status code of the HTTP response, if a positive value returned. A
   * negative value indicates a possible networking issue (retrying the request
   * is recommended).
   */
  int GetStatus() const;

  /**
   * @brief Error description getter method.
   * @return Error description of failed request.
   */
  const ErrorResponse& GetErrorResponse() const;

  /**
   * @brief List of all specific input field errors getter method.
   * @return List of input field errors.
   */
  const ErrorFields& GetErrorFields() const;

  /**
   * @brief Access token getter method.
   * @return String containing the new HERE Account client or user access token,
   * identifying the signed-in client or user.
   */
  const std::string& GetAccessToken() const;

  /**
   * @brief Access token type getter method.
   * @return String containing the access token type (always "bearer").
   */
  const std::string& GetTokenType() const;

  /**
   * @brief Refresh token getter method.
   * @return String containing a token which is used to obtain a new access
   * token using the refresh API. Refresh token is always issued as part of the
   * response to a user sign in operation.
   */
  const std::string& GetRefreshToken() const;

  /**
   * @brief Access token expiry time getter method.
   * @return Epoch time when the token is expiring, -1 for invalid.
   */
  time_t GetExpiryTime() const;

  /**
   * @brief Here account user identifier getter method.
   * @return String containing the HERE Account user ID.
   */
  const std::string& GetUserIdentifier() const;

 private:
  friend class SignInUserResult;
  friend class AuthenticationClient;

  SignInResult(std::shared_ptr<SignInResultImpl> impl);

 private:
  std::shared_ptr<SignInResultImpl> impl_;
};
}  // namespace authentication
}  // namespace olp
