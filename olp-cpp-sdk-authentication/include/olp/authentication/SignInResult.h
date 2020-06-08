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

#include <chrono>
#include <ctime>
#include <memory>
#include <string>

#include "AuthenticationApi.h"
#include "ErrorResponse.h"

namespace olp {
namespace authentication {
class SignInResultImpl;

/**
 * @brief A response to a client or user sign-in operation.
 */
class AUTHENTICATION_API SignInResult {
 public:
  /**
   * @brief A default destructor.
   */
  virtual ~SignInResult();

  /**
   * @brief A default constructor.
   */
  SignInResult() noexcept;

  /**
   * @brief Gets an HTTP status code.
   *
   * @return The status code of the HTTP response if a positive value is
   * returned. A negative value indicates a possible networking error. If you
   * get the negative value, retry the request.
   */
  int GetStatus() const;

  /**
   * @brief Gets an error description.
   *
   * @return The error description of the failed request.
   */
  const ErrorResponse& GetErrorResponse() const;

  /**
   * @brief Gets a list of all specific input field errors.
   *
   * @return The list of input field errors.
   */
  const ErrorFields& GetErrorFields() const;

  /**
   * @brief Gets an access token.
   *
   * @return The string that contains the new HERE Account client or user access
   * token that identifies the signed-in client or user.
   */
  const std::string& GetAccessToken() const;

  /**
   * @brief Gets the access token type.
   *
   * @return The string containing the access token type (always a bearer
   * token).
   */
  const std::string& GetTokenType() const;

  /**
   * @brief Gets a refresh token.
   *
   * @return The string with the token that is used to get a new access
   * token via the refresh API. The refresh token is always issued as a part of
   * a response to a user sign-in operation.
   */
  const std::string& GetRefreshToken() const;

  /**
   * @brief Gets the access token expiry time.
   *
   * @return The Epoch time when the token expires, or -1 if the token is
   * invalid.
   */
  time_t GetExpiryTime() const;

  /**
   * @brief Gets the access token expiry time.
   *
   * @return The expiry time of the access token.
   */
  std::chrono::seconds GetExpiresIn() const;

  /**
   * @brief Gets the HERE Account user identifier.
   *
   * @return The string that contains the HERE Account user ID.
   */
  const std::string& GetUserIdentifier() const;

  /**
   * @brief Gets the scope that is assigned to the access token.
   *
   * @return The string that contains the scope assigned to the access token.
   */
  const std::string& GetScope() const;

 private:
  friend class SignInUserResult;
  friend class AuthenticationClientImpl;

  SignInResult(std::shared_ptr<SignInResultImpl> impl);

 private:
  std::shared_ptr<SignInResultImpl> impl_;
};
}  // namespace authentication
}  // namespace olp
