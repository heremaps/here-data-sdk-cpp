/*
 * Copyright (C) 2026 HERE Europe B.V.
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
#include <string>

#include <olp/authentication/AuthenticationApi.h>
#include <olp/core/porting/optional.h>

namespace olp {
namespace authentication {

/**
 * @brief Properties used to sign in via mTLS client certificate.
 */
struct AUTHENTICATION_API MtlsProperties {
  /**
   * @brief (Required) The client certificate PEM blob presented during the
   * TLS handshake.
   */
  std::string client_cert_pem;

  /**
   * @brief (Required) The client private key PEM blob matching
   * `client_cert_pem`.
   */
  std::string client_key_pem;

  /**
   * @brief (Optional) The CA certificate PEM blob used to verify the token
   * endpoint's server certificate, in addition to the system default CAs.
   */
  std::string ca_cert_pem;

  /**
   * @brief (Optional) The number of seconds left before the access token
   * expires.
   *
   * Ignored if it is zero or greater than the default expiration time
   * supported by the mTLS token endpoint.
   */
  std::chrono::seconds expires_in{0};

  /**
   * @brief (Optional) The project scope (HRN) to be assigned to the access
   * token.
   */
  porting::optional<std::string> scope{porting::none};
};

}  // namespace authentication
}  // namespace olp
