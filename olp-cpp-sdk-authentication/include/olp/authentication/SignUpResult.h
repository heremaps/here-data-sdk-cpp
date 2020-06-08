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
class SignUpResultImpl;

/**
 * @brief A response to the sign-up operation with your HERE Account user ID.
 *
 * Contains the following results of your sign-up request:
 * status ( \ref GetStatus ), user ID ( \ref GetUserIdentifier ),
 * and, in case of an unsuccessful sign-out operation,
 * the error description ( \ref GetErrorResponse ) and
 * input fields errors ( \ref GetErrorFields ).
 */
class AUTHENTICATION_API SignUpResult {
 public:
  /**
   * @brief A default destructor.
   */
  virtual ~SignUpResult();

  /**
   * @brief A default constructor.
   */
  SignUpResult() noexcept;

  /**
   * @brief Gets the HTTP status code.
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
   * @brief Gets the HERE Account user identifier.
   *
   * @return The string that contains the HERE Account user ID.
   */
  const std::string& GetUserIdentifier() const;

 private:
  friend class AuthenticationClientImpl;

  SignUpResult(std::shared_ptr<SignUpResultImpl> impl);

 private:
  std::shared_ptr<SignUpResultImpl> impl_;
};
}  // namespace authentication
}  // namespace olp
