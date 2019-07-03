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
 * @brief The NetworkProxySettings struct contains proxy configuration settings
 * for the network layer.
 * @note only HTTP type proxies are supported.
 */
struct AUTHENTICATION_API NetworkProxySettings {
  /**
   * @brief Proxy server address. Only HTTP type proxies are supported.
   * @note The 'http://' prefix of the host is implied and should NOT be part of
   * the host string.
   */
  std::string host;

  /**
   * @brief Proxy port number
   */
  std::uint16_t port;

  /**
   * @brief Proxy user name
   */
  std::string username;

  /**
   * @brief Proxy user password
   */
  std::string password;
};
}  // namespace authentication
}  // namespace olp
