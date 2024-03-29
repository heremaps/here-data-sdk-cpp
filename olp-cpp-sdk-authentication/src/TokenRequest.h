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

#include <chrono>
#include <cstdint>

#include <olp/authentication/AuthenticationApi.h>

namespace olp {
namespace authentication {
/**
 * @brief Holds the parameters of the OAuth2.0 Authorization
 * Grant request.
 */
class TokenRequest {
 public:
  /**
   * @brief Creates the `TokenRequest` instance.
   *
   * @param expires_in (Optional) The number of seconds left before the new
   * access token expires.
   */
  TokenRequest(
      const std::chrono::seconds& expires_in = std::chrono::seconds(0));

  /**
   * @brief The number of seconds left before the token expires.
   *
   * Ignored if it is zero or greater than the default expiration time supported
   * by the access token endpoint.
   *
   * @return The number of seconds left before the token expires.
   */
  std::chrono::seconds GetExpiresIn() const;

 private:
  std::chrono::seconds expires_in_;
};

}  // namespace authentication
}  // namespace olp
