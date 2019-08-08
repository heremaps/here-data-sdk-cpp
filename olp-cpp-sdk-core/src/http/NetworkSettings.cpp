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

#include "olp/core/http/NetworkSettings.h"

namespace olp {
namespace http {

std::size_t NetworkSettings::GetRetries() const { return retries_; }

int NetworkSettings::GetConnectionTimeout() const {
  return connection_timeout_;
}

int NetworkSettings::GetTransferTimeout() const { return transfer_timeout_; }

const NetworkProxySettings& NetworkSettings::GetProxySettings() const {
  return proxy_settings_;
}

NetworkSettings& NetworkSettings::WithRetries(std::size_t retries) {
  retries_ = retries;
  return *this;
}

NetworkSettings& NetworkSettings::WithConnectionTimeout(int timeout) {
  connection_timeout_ = timeout;
  return *this;
}

NetworkSettings& NetworkSettings::WithTransferTimeout(int timeout) {
  transfer_timeout_ = timeout;
  return *this;
}

NetworkSettings& NetworkSettings::WithProxySettings(
    NetworkProxySettings settings) {
  proxy_settings_ = std::move(settings);
  return *this;
}

}  // namespace http
}  // namespace olp
