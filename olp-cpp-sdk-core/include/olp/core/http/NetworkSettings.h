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
#include <string>

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
  std::size_t GetRetries() const;

  /**
   * @brief Sets the maximum number of retries for the HTTP request.
   *
   * @param[in] retries The maximum number of retries for HTTP request.
   *
   * @return A reference to *this.
   */
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

 private:
  /// The maximum number of retries for the HTTP request.
  std::size_t retries_{3};
  /// The connection timeout.
  std::chrono::milliseconds connection_timeout_ = std::chrono::seconds(60);
  /// The transfer timeout.
  std::chrono::milliseconds transfer_timeout_ = std::chrono::seconds(30);
  /// The network proxy settings.
  NetworkProxySettings proxy_settings_;
};

}  // namespace http
}  // namespace olp
