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

#include "NetworkProxy.h"

namespace olp {
namespace network {
/**
 * @brief The NetworkConfig class contains configuration for the network
 */
class CORE_API NetworkConfig {
 public:
  /**
   * @brief NetworkConfig constructor
   * @param connect_timeout - connection timeout in seconds
   * @param transfer_timeout - transmit timeout in seconds
   * @param dont_verify_certificate - allow invalid certificates
   * @param retries - how many time network interface retries transmission
   * @param skip_content_when_error - skip or don't skip error message if error
   * occurs
   */
  NetworkConfig(int connect_timeout = 60, int transfer_timeout = 30,
                bool dont_verify_certificate = false, size_t retries = 3,
                bool skip_content_when_error = false);

  /**
   * @brief Set network timeouts
   * @param connect_timeout - connection timeout in seconds
   * @param transfer_timeout - transmit timeout in seconds
   */
  void SetTimeouts(int connect_timeout, int transfer_timeout);

  /**
   * @brief Skip or don't skip error message if error occurs
   * @param state = if true, then skip HTTP content if case of error
   */
  void SetSkipContentWhenError(bool state);

  /**
   * @brief Set how many times network modules retries transmission in case of
   * error
   * @param retries - amount of retries
   */
  void SetRetries(size_t retries);

  /**
   * @brief Set proxy configuration for the network connection
   * @param proxy - network proxy
   */
  void SetProxy(const NetworkProxy& Proxy);

  /**
   * @brief Get connectTimeout
   * @return connection timeout in seconds
   */
  int ConnectTimeout() const;

  /**
   * @brief Get transferTimeout
   * @return transfer timeout in seconds
   */
  int TransferTimeout() const;

  /**
   * @brief Check if content is skipped in case of error
   * @return true if content is skipped in case of error
   */
  bool SkipContentWhenError() const;

  /**
   * @brief Get how many times network modules retries transmission in case of
   * error
   * @return amount of retries
   */
  size_t GetRetries() const;

  /**
   * @brief Get proxy
   * @return system network proxy
   */
  const NetworkProxy& Proxy() const;

  /**
   * @brief setNetworkInterface - specifies outgoing network interface to bind
   * to
   * @param network_interface
   */
  void SetNetworkInterface(const std::string& network_interface);

  /**
   * @brief getNetworkInterface
   * @return specified network interface name
   */
  const std::string& GetNetworkInterface() const;

  /**
   * @brief setCaCert - specifies path to CA certificate bundle
   * @param ca_cert
   */
  void SetCaCert(const std::string& ca_cert);

  /**
   * @brief getCaCert
   * @return specified path to CA certificate bundle
   */
  const std::string& GetCaCert() const;

  /**
   * @brief Enables auto-decompression (only works if the used network protocol
   * supports this option, such as libcurl)
   * @param enable_auto_decompression true, if received data should be
   * automatically decompressed, else false
   */
  void EnableAutoDecompression(bool enable_auto_decompression);

  /**
   * @brief Check if auto-decompression is enabled.
   * @return true, if received data should be automatically decompressed, else
   * false
   */
  bool IsAutoDecompressionEnabled() const;

 private:
  size_t retries_{0};
  int connect_timeout_{0};
  int transfer_timeout_{0};
  bool skip_content_when_error_{false};
  bool enable_auto_decompression_{true};
  NetworkProxy proxy_;
  std::string network_interface_;
  std::string ca_cert_;
};

}  // namespace network
}  // namespace olp
