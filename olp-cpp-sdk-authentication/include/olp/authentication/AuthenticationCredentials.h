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

#include <string>

#include "AuthenticationApi.h"

namespace olp {
namespace authentication {
/**
 * @brief The AuthenticationCredentials represent the access keys issued for the
 * client by the HERE Account as part of the onboarding or support process
 * on the developer portal.
 */
class AUTHENTICATION_API AuthenticationCredentials {
 public:
  AuthenticationCredentials() = delete;

  /**
   * @brief Constructor
   * @param key The client access key identifier.
   * @param secret The client access key secret.
   */
  AuthenticationCredentials(std::string key, std::string secret);

  /**
   * @brief Copy Constructor
   * @param other object to copy from
   */
  AuthenticationCredentials(const AuthenticationCredentials& other);

  /**
   * @brief access the key of this AuthenticationCredentials object
   * @return a reference to the key member
   */
  const std::string& GetKey() const;

  /**
   * @brief access the secret of this AuthenticationCredentials object
   * @return a reference to the secret member
   */
  const std::string& GetSecret() const;

 private:
  std::string key_;
  std::string secret_;
};

}  // namespace authentication
}  // namespace olp
