/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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
#include <string>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/http/NetworkProxySettings.h>
#include <olp/core/http/NetworkTypes.h>
#include <olp/core/porting/deprecated.h>

namespace olp {
namespace http {

/**
 * @brief Contains a configuration for the network.
 */
class CORE_API NetworkSettings final {
 public:
  /**
   * @brief Gets the maximum number of retries for the HTTP request.
   *
   * @return The maximum number of retries for the HTTP request.
   */
  OLP_SDK_DEPRECATED("Will be removed by 04.2024")
  std::size_t GetRetries() const;

  /**
   * @brief Sets the maximum number of retries for the HTTP request.
   *
   * @param[in] retries The maximum number of retries for HTTP request.
   *
   * @return A reference to *this.
   */
  OLP_SDK_DEPRECATED("Will be removed by 04.2024")
  NetworkSettings& WithRetries(std::size_t retries);

  /**
   * @brief Gets the connection timeout in seconds.
   *
   * @return The connection timeout in seconds.
   */
  OLP_SDK_DEPRECATED(
      "Will be removed by 04.2024, use GetConnectionTimeoutDuration() instead")
  int GetConnectionTimeout() const;

  /**
   * @brief Gets the connection timeout.
   *
   * @return The connection timeout.
   */
  std::chrono::milliseconds GetConnectionTimeoutDuration() const;

#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  /**
   * @brief Gets the background connection timeout.
   *
   * @return The background connection timeout.
   */
  std::chrono::milliseconds GetBackgroundConnectionTimeoutDuration() const;
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD

  /**
   * @brief Sets the connection timeout in seconds.
   *
   * @param[in] timeout The connection timeout in seconds.
   *
   * @return A reference to *this.
   */
  OLP_SDK_DEPRECATED(
      "Will be removed by 04.2024, use "
      "WithConnectionTimeout(std::chrono::milliseconds) instead")
  NetworkSettings& WithConnectionTimeout(int timeout);

  /**
   * @brief Sets the connection timeout.
   *
   * @param[in] timeout The connection timeout.
   *
   * @return A reference to *this.
   */
  NetworkSettings& WithConnectionTimeout(std::chrono::milliseconds timeout);

#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  /**
   * @brief Sets the background connection timeout.
   *
   * @param[in] timeout The background connection timeout.
   *
   * @return A reference to *this.
   */
  NetworkSettings& WithBackgroundConnectionTimeout(
      std::chrono::milliseconds timeout);
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD

  /**
   * @brief Gets the transfer timeout in seconds.
   *
   * @return The transfer timeout in seconds.
   */
  OLP_SDK_DEPRECATED(
      "Will be removed by 04.2024, use GetTransferTimeoutDuration() instead")
  int GetTransferTimeout() const;

  /**
   * @brief Gets the transfer timeout.
   *
   * @return The transfer timeout.
   */
  std::chrono::milliseconds GetTransferTimeoutDuration() const;

  /**
   * @brief Sets the transfer timeout in seconds.
   *
   * @param[in] timeout The transfer timeout in seconds.
   *
   * @return A reference to *this.
   */
  OLP_SDK_DEPRECATED(
      "Will be removed by 04.2024, use "
      "WithTransferTimeout(std::chrono::milliseconds) instead")
  NetworkSettings& WithTransferTimeout(int timeout);

  /**
   * @brief Gets max lifetime (since creation) allowed for reusing a connection.
   *
   * @return The lifetime.
   */
  std::chrono::seconds GetMaxConnectionLifetime() const;

  /**
   * @brief Sets max lifetime (since creation) allowed for reusing a connection.
   * Supported only for CURL implementation. If set to 0, this behavior is
   * disabled: all connections are eligible for reuse.
   *
   * @return A reference to *this.
   */
  NetworkSettings& WithMaxConnectionLifetime(std::chrono::seconds lifetime);

  /**
   * @brief Sets the transfer timeout.
   *
   * @param[in] timeout The transfer timeout.
   *
   * @return A reference to *this.
   */
  NetworkSettings& WithTransferTimeout(std::chrono::milliseconds timeout);

  /**
   * @brief Gets the proxy settings.
   *
   * @return The proxy settings.
   */
  const NetworkProxySettings& GetProxySettings() const;

  /**
   * @brief Sets the proxy settings.
   *
   * @param[in] settings The proxy settings.
   *
   * @return A reference to *this.
   */
  NetworkSettings& WithProxySettings(NetworkProxySettings settings);

  /**
   * @brief Gets the DNS list.
   *
   * @return The DNS list.
   */
  const std::vector<std::string>& GetDNSServers() const;

  /**
   * @brief Sets the DNS servers to use. Works only with CURL implementation.
   * The order is important.To reduce response time make sure that most probably
   * servers are at the beginning.
   *
   * Note: This list replaces any other mechanism to retrieve DNS list.
   *
   * @param[in] dns_servers The DNS list.
   *
   * @return A reference to *this.
   */
  NetworkSettings& WithDNSServers(std::vector<std::string> dns_servers);

 private:
  /// The maximum number of retries for the HTTP request.
  std::size_t retries_{3};
  /// The connection timeout.
  std::chrono::milliseconds connection_timeout_ = std::chrono::seconds(60);
#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  /// The background connection timeout.
  std::chrono::milliseconds background_connection_timeout_ =
      std::chrono::seconds(600);
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  /// The transfer timeout.
  std::chrono::milliseconds transfer_timeout_ = std::chrono::seconds(30);
  /// The max lifetime since creation allowed for reusing a connection.
  std::chrono::seconds connection_lifetime_{0};
  /// The network proxy settings.
  NetworkProxySettings proxy_settings_;
  /// The additional DNS servers
  std::vector<std::string> dns_servers_;
};

}  // namespace http
}  // namespace olp
