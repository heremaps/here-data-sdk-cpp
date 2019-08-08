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
#include <string>

#include <olp/core/CoreApi.h>

namespace olp {
namespace http {

/**
 * @brief This class contains proxy configuration for the network interface to
 * be applied per request.
 */
class CORE_API NetworkProxySettings final {
 public:
  /// The proxy type.
  enum class Type {
    NONE,     ///< Don't use proxy.
    HTTP,     ///< HTTP proxy as in https://www.ietf.org/rfc/rfc2068.txt.
    SOCKS4,   ///< SOCKS4 proxy.
    SOCKS4A,  ///< SOCKS4a Proxy. Proxy resolves URL hostname.
    SOCKS5,   ///< SOCKS5 proxy.
    SOCKS5_HOSTNAME,  ///< SOCKS5 Proxy. Proxy resolves URL hostname.
  };

  /**
   * @brief Get proxy type.
   * @return proxy type.
   */
  Type GetType() const;

  /**
   * @brief Set proxy type.
   * @param[in] type Proxy type.
   * @return reference to *this.
   */
  NetworkProxySettings& WithType(Type type);

  /**
   * @brief Get proxy hostname.
   * @return proxy hostname.
   */
  const std::string& GetHostname() const;

  /**
   * Set proxy hostname.
   * @param[in] hostname Proxy hostname.
   * @return reference to *this.
   */
  NetworkProxySettings& WithHostname(std::string hostname);

  /**
   * @brief Get proxy port.
   * @return proxy port.
   */
  std::uint16_t GetPort() const;

  /**
   * @brief Set proxy port.
   * @param[in] port Proxy port.
   * @return reference to *this.
   */
  NetworkProxySettings& WithPort(std::uint16_t port);

  /**
   * @brief Get username.
   * @return username.
   */
  const std::string& GetUsername() const;

  /**
   * @brief Set username.
   * @param[in] username
   * @return reference to *this.
   */
  NetworkProxySettings& WithUsername(std::string username);

  /**
   * @brief Get password.
   * @return password.
   */
  const std::string& GetPassword() const;

  /**
   * @brief Set password.
   * @param[in] password
   * @return reference to *this.
   */
  NetworkProxySettings& WithPassword(std::string password);

 private:
  /// The type of the proxy.
  Type type_{Type::NONE};
  /// The port of the proxy.
  std::uint16_t port_{0};
  /// The hostname of the proxy.
  std::string hostname_;
  /// The username for the proxy.
  std::string username_;
  /// The password for the proxy.
  std::string password_;
};

}  // namespace http
}  // namespace olp
