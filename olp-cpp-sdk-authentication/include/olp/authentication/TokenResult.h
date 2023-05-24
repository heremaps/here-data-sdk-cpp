/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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
#include <string>

#include "AuthenticationApi.h"
#include "ErrorResponse.h"

namespace olp {
namespace authentication {
/**
 * @brief A parsed response received from the OAuth2.0 token endpoint.
 *
 * You can get the following information: the access token issued by
 * the authorization server (`GetAccessToken`), and its expiry time
 * (`GetExpiryTime`).
 */
class AUTHENTICATION_API TokenResult {
 public:
  /**
   * @brief Creates the `TokenResult` instance.
   *
   * @param access_token The access token issued by the authorization server.
   * @param expiry_time The Epoch time when the token expires, or -1 if
   * the token is invalid.
   */
  TokenResult(std::string access_token, time_t expiry_time);

  /**
   * @brief Creates the `TokenResult` instance.
   *
   * @param access_token The access token issued by the authorization server.
   * @param expires_in The expiry time of the access token.
   */
  TokenResult(std::string access_token, std::chrono::seconds expires_in);
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

 private:
  std::string access_token_;
  time_t expiry_time_{};
  std::chrono::seconds expires_in_{};
};

}  // namespace authentication
}  // namespace olp
