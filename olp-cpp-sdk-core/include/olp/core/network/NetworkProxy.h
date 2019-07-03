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
namespace network {
/**
 * @brief The NetworkProxy class contains proxy configuration for the network
 */
class CORE_API NetworkProxy {
 public:
  /**
   * @brief The proxy type
   */
  enum class Type : int {
    /**
     * @brief Invalid type
     */
    Unknown = -1,
    /**
     * @brief The http/https proxy
     */
    Http = 0,
    /**
     * @brief The socks 4 proxy
     */
    Socks4 = 4,
    /**
     * @brief The socks 4A proxy
     */
    Socks4A = 5,
    /**
     * @brief The socks 5 proxy
     */
    Socks5 = 6,
    /**
     * @brief The socks proxy with hostname
     */
    Socks5Hostname = 7
  };

 public:
  /**
   * @brief NetworkProxy constructor
   *
   * @param name - name of proxy server
   * @param port - proxy port number
   * @param type - proxy type, default to Http
   * @param user_name - proxy user name
   * @param user_password - proxy user password
   */
  NetworkProxy(const std::string& name = std::string(), std::uint16_t port = 0,
               Type type = Type::Unknown,
               const std::string& user_name = std::string(),
               const std::string& user_password = std::string());

  /**
   * @brief Check if the proxy is valid
   * @return true if valid, otherwise false
   */
  bool IsValid() const;

  /**
   * @brief Get proxy name
   * @return proxy server name
   */
  const std::string& Name() const;

  /**
   * @brief Get proxy port
   * @return proxy port number
   */
  std::uint16_t Port() const;

  /**
   * @brief Get proxy type
   * @return proxy type
   */
  Type ProxyType() const;

  /**
   * @brief Get proxy user name string
   * @return proxy user name
   */
  const std::string& UserName() const;

  /**
   * @brief Get user password string
   * @return proxy user password
   */
  const std::string& UserPassword() const;

 private:
  std::string _name;
  std::uint16_t port_{0};
  Type type_{Type::Unknown};
  std::string user_name_;
  std::string user_password_;
};

}  // namespace network
}  // namespace olp
