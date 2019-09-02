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
 * @brief The SignUpResponse class represents the response to a user sign-up
 * operation.
 */
class AUTHENTICATION_API SignUpResult {
 public:
  /**
   * @brief Destructor
   */
  virtual ~SignUpResult();

  /**
   * @brief Constructor
   */
  SignUpResult() noexcept;

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
   * @brief Here account user identifier getter method.
   * @return String containing the HERE Account user ID.
   */
  const std::string& GetUserIdentifier() const;

 private:
  friend class AuthenticationClient;

  SignUpResult(std::shared_ptr<SignUpResultImpl> impl);

 private:
  std::shared_ptr<SignUpResultImpl> impl_;
};
}  // namespace authentication
}  // namespace olp
