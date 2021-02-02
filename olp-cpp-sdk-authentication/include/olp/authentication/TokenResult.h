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

#include <olp/core/porting/deprecated.h>

#include <chrono>
#include <ctime>
#include <string>

#include "AuthenticationApi.h"
#include "ErrorResponse.h"

namespace olp {
namespace authentication {
/**
 * @brief A parsed response received from the OAuth2.0 token endpoint.
 *
 * You can get the following information: the access token issued by
 * the authorization server ( \ref GetAccessToken ), its expiry time
 * ( \ref GetExpiryTime ), HTTP status code ( \ref GetHttpStatus ),
 * and error description ( \ref GetErrorResponse ) in case of a failure.
 */
class AUTHENTICATION_API TokenResult {
 public:
  /**
   * @brief Creates the `TokenResult` instance.
   *
   * @param access_token The access token issued by the authorization server.
   * @param expiry_time The Epoch time when the token expires, or -1 if
   * the token is invalid.
   * @param http_status The status code of the HTTP response.
   * @param error The error description of the request.
   */
  TokenResult(std::string access_token, time_t expiry_time, int http_status,
              ErrorResponse error);

  /**
   * @brief Creates the `TokenResult` instance.
   *
   * @param access_token The access token issued by the authorization server.
   * @param expires_in The expiry time of the access token.
   * @param http_status The status code of the HTTP response.
   * @param error The error description of the request.
   */
  TokenResult(std::string access_token, std::chrono::seconds expires_in,
              int http_status, ErrorResponse error);
  /**
   * @brief Creates the default `TokenResult` instance.
   */
  TokenResult() = default;

  /**
   * @brief Gets the access token issued by the authorization server.
   *
   * @return The access token issued by the authorization server.
   */
  const std::string& GetAccessToken() const;

  /**
   * @brief Gets the Epoch time when the token expires, or -1 if the token is
   * invalid.
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
   * @brief Gets the HTTP status code.
   *
   * @return The status code of the HTTP response if a positive value is
   * returned. A negative value indicates a possible networking error. If you
   * get the negative value, retry the request.
   */
  int GetHttpStatus() const;

  /**
   * @brief Gets an error description.
   *
   * @return The error description of the failed request.
   */
  ErrorResponse GetErrorResponse() const;

 private:
  std::string access_token_;
  time_t expiry_time_;
  std::chrono::seconds expires_in_;
  int http_status_;
  ErrorResponse error_;
};

}  // namespace authentication
}  // namespace olp
