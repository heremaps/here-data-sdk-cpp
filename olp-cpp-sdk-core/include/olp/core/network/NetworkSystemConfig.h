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

#include <cstdint>
#include <functional>
#include <string>

#include "NetworkProxy.h"

namespace olp {
namespace network {
/**
 * @brief The NetworkSystemConfig class contains system configuration for the
 * network
 * @note the network system configuration should only be set at SDK or
 * application level.
 */
class CORE_API NetworkSystemConfig {
 public:
  /**
   * @brief NetworkSystemConfig default constructor
   */
  NetworkSystemConfig() = default;

  /**
   * @brief NetworkSystemConfig constructor
   * @param system_proxy - network system proxy
   * @param certificate_path - path of Certificate Authority (CA) certificate
   */
  NetworkSystemConfig(const NetworkProxy& system_proxy,
                      const std::string& certificate_path = std::string());

  /**
   * @brief Set proxy configuration for the system or application
   * @param proxy - network proxy
   */
  void SetProxy(const NetworkProxy& GetProxy);

  /**
   * @brief Set path for Certificate Authority (CA) certificate
   * @param path - path of CA certificate
   */
  void SetCertificatePath(const std::string& path);

  /**
   * @brief Set alternative path for Certificate Authority (CA) certificate
   * (i.e. for storing updates without overwriting old certificates)
   * @param path - path of CA certificate
   */
  void SetAlternativeCertificatePath(const std::string& path);

  /**
   * @brief Get proxy
   * @return system network proxy
   */
  const NetworkProxy& GetProxy() const;

  /**
   * @brief Get path of CA certificate
   * @return path of CA certificate
   */
  const std::string& GetCertificatePath() const;

  /**
   * @brief Get alternative path of CA certificate (i.e. for storing updates
   * without overwriting old certificates)
   * @return alternative path of CA certificate
   */
  const std::string& GetAlternativeCertificatePath() const;

  /**
   * @brief Check if invalid certificates are allowed globally.
   * @see NETWORK_SSL_VERIFY environment variable
   * @return true if invalid certificates are allowed globally
   */
  bool DontVerifyCertificate() const;

 private:
  std::string certificate_path_;
  std::string alternative_certificate_path_;
  NetworkProxy proxy_;
};

}  // namespace network
}  // namespace olp
