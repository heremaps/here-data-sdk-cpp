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

#include <olp/core/logging/Log.h>
#include <olp/core/network/NetworkSystemConfig.h>
#include <olp/core/network/Settings.h>

#define LOGTAG "NETWORK"

namespace olp {
namespace network {
NetworkSystemConfig::NetworkSystemConfig(const NetworkProxy& system_proxy,
                                         const std::string& certificate_path)
    : certificate_path_(certificate_path), proxy_(system_proxy) {}

void NetworkSystemConfig::SetProxy(const NetworkProxy& proxy) {
  proxy_ = proxy;
}

void NetworkSystemConfig::SetCertificatePath(const std::string& path) {
  certificate_path_ = path;
}

void NetworkSystemConfig::SetAlternativeCertificatePath(
    const std::string& path) {
  alternative_certificate_path_ = path;
}

const NetworkProxy& NetworkSystemConfig::GetProxy() const { return proxy_; }

const std::string& NetworkSystemConfig::GetCertificatePath() const {
  return certificate_path_;
}

const std::string& NetworkSystemConfig::GetAlternativeCertificatePath() const {
  return alternative_certificate_path_;
}

bool NetworkSystemConfig::DontVerifyCertificate() const {
#if defined(NETWORK_SSL_VERIFY_OVERRIDE)
  // Allow overriding of Certificate verification for
  //  troubleshooting/development purposes
  if (Settings::GetEnvInt("NETWORK_SSL_VERIFY", -1) == 0) {
    EDGE_SDK_LOG_INFO(
        LOGTAG, "Network SSL verification disabled by NETWORK_SSL_VERIFY=0");
    return true;
  }
#endif  // NETWORK_SSL_VERIFY_OVERRIDE
  return false;
}

}  // namespace network
}  // namespace olp
