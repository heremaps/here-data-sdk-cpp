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

#include <cstdint>
#include <string>

#include <olp/core/CoreApi.h>

namespace olp {
namespace http {

/**
 * @brief Contains a proxy configuration for the network interface that
 * is applied per request.
 */
class CORE_API NetworkProxySettings final {
 public:
  /// The proxy type.
  enum class Type {
    NONE,             ///< Don't use the proxy.
    HTTP,             ///< HTTP proxy as in https://www.ietf.org/rfc/rfc2068.txt
    SOCKS4,           ///< SOCKS4 proxy.
    SOCKS4A,          ///< SOCKS4a proxy. Proxy resolves the URL hostname.
    SOCKS5,           ///< SOCKS5 proxy.
    SOCKS5_HOSTNAME,  ///< SOCKS5 Proxy. Proxy resolves the URL hostname.
  };

  /**
   * @brief Gets the proxy type.
   *
   * @return The proxy type.
   */
  Type GetType() const;

  /**
   * @brief Sets the proxy type.
   *
   * @param[in] type The proxy type.
   *
   * @return A reference to *this.
   */
  NetworkProxySettings& WithType(Type type);

  /**
   * @brief Gets the proxy hostname.
   *
   * @return The proxy hostname.
   */
  const std::string& GetHostname() const;

  /**
   * @brief Sets the proxy hostname.
   *
   * @param[in] hostname The proxy hostname.
   *
   * @return A reference to *this.
   */
  NetworkProxySettings& WithHostname(std::string hostname);

  /**
   * @brief Gets the proxy port.
   *
   * @return The proxy port.
   */
  std::uint16_t GetPort() const;

  /**
   * @brief Sets the proxy port.
   *
   * @param[in] port The proxy port.
   *
   * @return A reference to *this.
   */
  NetworkProxySettings& WithPort(std::uint16_t port);

  /**
   * @brief Gets the username.
   *
   * @return The username.
   */
  const std::string& GetUsername() const;

  /**
   * @brief Sets the username.
   *
   * @param[in] username The username.
   *
   * @return A reference to *this.
   */
  NetworkProxySettings& WithUsername(std::string username);

  /**
   * @brief Gets the password.
   *
   * @return The password.
   */
  const std::string& GetPassword() const;

  /**
   * @brief Sets the password.
   *
   * @param[in] password The password.
   *
   * @return A reference to *this.
   */
  NetworkProxySettings& WithPassword(std::string password);

 private:
  /// The proxy type.
  Type type_{Type::NONE};
  /// The proxy port.
  std::uint16_t port_{0};
  /// The proxy hostname.
  std::string hostname_;
  /// The proxy username.
  std::string username_;
  /// The proxy password.
  std::string password_;
};

}  // namespace http
}  // namespace olp
