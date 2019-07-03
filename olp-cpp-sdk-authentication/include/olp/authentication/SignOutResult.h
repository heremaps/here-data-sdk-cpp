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

#include "AuthenticationApi.h"
#include "ErrorResponse.h"

namespace olp {
namespace authentication {
class SignOutResultImpl;

/**
 * @brief The SignOutResult class represents the response to a user sign-out
 * operation.
 */
class AUTHENTICATION_API SignOutResult {
 public:
  /**
   * @brief Destructor
   */
  virtual ~SignOutResult();

  /**
   * @brief Constructor
   */
  SignOutResult() = default;

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

 private:
  friend class AuthenticationClient;

  SignOutResult(std::shared_ptr<SignOutResultImpl> impl);

 private:
  std::shared_ptr<SignOutResultImpl> impl_;
};
}  // namespace authentication
}  // namespace olp
