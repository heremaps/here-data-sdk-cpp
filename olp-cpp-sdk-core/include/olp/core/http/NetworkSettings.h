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
#include <olp/core/http/NetworkProxySettings.h>
#include <olp/core/http/NetworkTypes.h>

namespace olp {
namespace http {

/**
 * @brief This class contains configuration for the network.
 */
class CORE_API NetworkSettings final {
 public:
  /**
   * @brief Get maximum number of retries for HTTP request.
   * @return maximum number of retries for HTTP request.
   */
  std::size_t GetRetries() const;

  /**
   * @brief Set maximum number of retries for HTTP request.
   * @param[in] retries Maximum number of retries for HTTP request.
   * @return reference to *this.
   */
  NetworkSettings& WithRetries(std::size_t retries);

  /**
   * @brief Get connection timeout in seconds.
   * @return connection timeout in seconds.
   */
  int GetConnectionTimeout() const;

  /**
   * @brief Set connection timeout in seconds.
   * @param[in] timeout Connection timeout in seconds.
   * @return reference to *this.
   */
  NetworkSettings& WithConnectionTimeout(int timeout);

  /**
   * @brief Get transfer timeout in seconds.
   * @return transfer timeout in seconds.
   */
  int GetTransferTimeout() const;

  /**
   * @brief Set transfer timeout in seconds.
   * @param[in] timeout Transfer timeout in seconds.
   * @return reference to *this.
   */
  NetworkSettings& WithTransferTimeout(int timeout);

  /**
   * @brief Get proxy settings.
   * @return proxy settings.
   */
  const NetworkProxySettings& GetProxySettings() const;

  /**
   * @brief Set proxy settings.
   * @param[in] settings Proxy settings.
   * @return reference to *this.
   */
  NetworkSettings& WithProxySettings(NetworkProxySettings settings);

 private:
  /// Maximum number of retries for HTTP request.
  std::size_t retries_{3};
  /// Connection timeout in seconds.
  int connection_timeout_{60};
  /// Transfer timeout in seconds.
  int transfer_timeout_{30};
  /// Network proxy settings.
  NetworkProxySettings proxy_settings_;
};

}  // namespace http
}  // namespace olp
