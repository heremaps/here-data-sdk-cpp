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
#include <chrono>

#include "BaseResult.h"

namespace olp {
namespace authentication {
class SignInResultImpl : public BaseResult {
 public:
  SignInResultImpl() noexcept;

  SignInResultImpl(
      int status, std::string error,
      std::shared_ptr<rapidjson::Document> json_document = nullptr) noexcept;

  ~SignInResultImpl() override;

  /**
   * @brief Access token getter method
   * @return string containing the new HERE Account user access token,
   * identifying the signed-in user.
   */
  const std::string& GetAccessToken() const;

  /**
   * @brief Access token type getter method
   * @return string containing the access token type (always "bearer").
   */
  const std::string& GetTokenType() const;

  /**
   * @brief Refresh token getter method
   * @return string containing a token which is used to obtain a new access
   * token using the refresh API. Refresh token is always issued unless
   * explicitly stated in the individual SignIn in question.
   */
  const std::string& GetRefreshToken() const;

  /**
   * @brief Expire time of the access token
   * @return time when the token is expiring, -1 for invalid
   */
  time_t GetExpiryTime() const;

    /**
   * @brief Gets the access token expiry time in seconds.
   * @return Duration for which token stays valid.
   */
  std::chrono::seconds GetExpiresIn() const;

  /**
   * @brief Here account user identifier
   * @return string containing the HERE Account user ID.
   */
  const std::string& GetUserIdentifier() const;

  /**
   * @brief is_valid
   * @return True if response is valid
   */
  bool IsValid() const override;

  const std::string& GetScope() const;

 private:
  bool is_valid_;

 protected:
  std::string access_token_;
  std::string token_type_;
  time_t expiry_time_;
  std::chrono::seconds expires_in_;

  std::string refresh_token_;
  std::string user_identifier_;
  std::string scope_;
};
}  // namespace authentication
}  // namespace olp
