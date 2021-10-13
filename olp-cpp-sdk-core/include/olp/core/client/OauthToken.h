/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <olp/core/CoreApi.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>

namespace olp {
namespace client {
/**
 * @brief A parsed response received from the OAuth2.0 token endpoint.
 *
 * You can get the following information: the access token issued by
 * the authorization server ( \ref GetAccessToken ) and its expiry time
 * ( \ref GetExpiryTime ).
 */
class CORE_API OauthToken {
 public:
  /**
   * @brief Creates the `OauthToken` instance.
   *
   * @param access_token The access token issued by the authorization server.
   * @param expiry_time The Epoch time when the token expires.
   */
  OauthToken(std::string access_token, time_t expiry_time);

  /**
   * @brief Creates the `OauthToken` instance.
   *
   * @param access_token The access token issued by the authorization server.
   * @param expires_in The expiry time of the access token.
   */
  OauthToken(std::string access_token, std::chrono::seconds expires_in);

  /**
   * @brief Creates the default `OauthToken` instance.
   */
  OauthToken() = default;

  /**
   * @brief Gets the access token issued by the authorization server.
   *
   * @return The access token issued by the authorization server.
   */
  const std::string& GetAccessToken() const;

  /**
   * @brief Gets the Epoch time when the token expires.
   *
   * @return The Epoch time when the token expires.
   */
  time_t GetExpiryTime() const;

  /**
   * @brief Gets the number of seconds the token is still valid for.
   *
   * @return The number of seconds the token is still valid for.
   */
  std::chrono::seconds GetExpiresIn() const;

 private:
  std::string access_token_;
  std::chrono::seconds expires_in_;
  time_t expiry_time_;
};

using OauthTokenResponse = ApiResponse<OauthToken, ApiError>;

}  // namespace client
}  // namespace olp
