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
#include <string>

#include "AuthenticationApi.h"
#include "ErrorResponse.h"

namespace olp {
namespace authentication {
/**
 * @brief The TokenResponse class represents a parsed response received from an
 * OAuth2.0 token endpoint.
 */
class AUTHENTICATION_API TokenResult {
 public:
  /**
   * @brief Constructor
   * @param access_token The access token issued by the authorization server.
   * @param expiry_time Epoch time when the token is expiring, -1 for invalid.
   * @param http_status Status code of the HTTP response, if a positive value
   * returned. A negative value indicates a possible networking issue (retrying
   * the request is recommended)
   * @param error Error description for the request.
   */
  TokenResult(std::string access_token, time_t expiry_time, int http_status,
              ErrorResponse error);

  /**
   * @brief Constructor
   */
  TokenResult() = default;

  /**
   * @brief Copy Constructor
   * @param other the object to copy from
   */
  TokenResult(const TokenResult& other);

  /**
   * @brief Gets the access token issued by the authorization server.
   * @return access token issued by the authorization server.
   */
  const std::string& GetAccessToken() const;

  /**
   * @brief Gets the Epoch time when the token is expiring, -1 for invalid.
   * @return Epoch time when the token is expiring, -1 for invalid.
   */
  time_t GetExpiryTime() const;

  /**
   * @brief Status code of the HTTP response, if a positive value returned. A
   * negative value indicates a possible networking issue (retrying the request
   * is recommended).
   * @return Status code of the HTTP response
   */
  int GetHttpStatus() const;

  /**
   * @brief Error description for the request.
   * @return Error description for the request.
   */
  ErrorResponse GetErrorResponse() const;

 private:
  std::string access_token_;
  time_t expiry_time_;
  int http_status_;
  ErrorResponse error_;
};

}  // namespace authentication
}  // namespace olp
