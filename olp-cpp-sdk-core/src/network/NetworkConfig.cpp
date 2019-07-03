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

#include "olp/core/network/NetworkConfig.h"

namespace olp {
namespace network {
NetworkConfig::NetworkConfig(int connect_timeout,
                             int transfer_timeout,
                             bool /*dont_verify_certificate*/, size_t retries,
                             bool skip_content_when_error)
    : retries_(retries),
      connect_timeout_(connect_timeout),
      transfer_timeout_(transfer_timeout),
      skip_content_when_error_(skip_content_when_error) {}

void NetworkConfig::SetTimeouts(int connect_timeout,
                                int transfer_timeout) {
  connect_timeout_ = connect_timeout;
  transfer_timeout_ = transfer_timeout;
}

void NetworkConfig::SetSkipContentWhenError(bool state) {
  skip_content_when_error_ = state;
}

void NetworkConfig::SetRetries(size_t retries) { retries_ = retries; }

void NetworkConfig::SetNetworkInterface(const std::string& network_interface) {
  network_interface_ = network_interface;
}

const std::string& NetworkConfig::GetNetworkInterface() const {
  return network_interface_;
}

void NetworkConfig::SetCaCert(const std::string& ca_cert) {
  ca_cert_ = ca_cert;
}

const std::string& NetworkConfig::GetCaCert() const { return ca_cert_; }

void NetworkConfig::SetProxy(const NetworkProxy& proxy) { proxy_ = proxy; }

int NetworkConfig::ConnectTimeout() const { return connect_timeout_; }

int NetworkConfig::TransferTimeout() const { return transfer_timeout_; }

bool NetworkConfig::SkipContentWhenError() const {
  return skip_content_when_error_;
}

size_t NetworkConfig::GetRetries() const { return retries_; }

const NetworkProxy& NetworkConfig::Proxy() const { return proxy_; }

void NetworkConfig::EnableAutoDecompression(bool enable_auto_decompression) {
  enable_auto_decompression_ = enable_auto_decompression;
}

bool NetworkConfig::IsAutoDecompressionEnabled() const {
  return enable_auto_decompression_;
}

}  // namespace network
}  // namespace olp
