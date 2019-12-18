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
#include <cstdint>

#include "AuthenticationApi.h"
#include <olp/core/porting/deprecated.h>

namespace olp {
namespace authentication {
/**
 * @brief The TokenRequest class holds parameters of an OAuth2.0 Authorization
 * Grant Request.
 */
class AUTHENTICATION_API OLP_SDK_DEPRECATED("Will be removed in 04.2020")
    TokenRequest {
 public:
  /**
   * @brief Constructor.
   * @param expiresIn Optional. The number of seconds until the new access token
   * expires.
   */
  TokenRequest(const std::chrono::seconds& expires_in = std::chrono::seconds(0));

  /**
   * @brief The number of seconds before token expires. Ignored if zero or
   * greater than default expiration supported by the token endpoint.
   * @return Number of seconds before token expires.
   */
  std::chrono::seconds GetExpiresIn() const;

 private:
  std::chrono::seconds expires_in_;
};

}  // namespace authentication
}  // namespace olp
