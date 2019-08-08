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

#include "olp/core/http/NetworkProxySettings.h"

namespace olp {
namespace http {

const std::string& NetworkProxySettings::GetHostname() const {
  return hostname_;
}

std::uint16_t NetworkProxySettings::GetPort() const { return port_; }

NetworkProxySettings::Type NetworkProxySettings::GetType() const {
  return type_;
}

const std::string& NetworkProxySettings::GetUsername() const {
  return username_;
}

const std::string& NetworkProxySettings::GetPassword() const {
  return password_;
}

NetworkProxySettings& NetworkProxySettings::WithHostname(std::string hostname) {
  hostname_ = std::move(hostname);
  return *this;
}

NetworkProxySettings& NetworkProxySettings::WithPort(std::uint16_t port) {
  port_ = port;
  return *this;
}

NetworkProxySettings& NetworkProxySettings::WithType(Type type) {
  type_ = type;
  return *this;
}

NetworkProxySettings& NetworkProxySettings::WithUsername(std::string username) {
  username_ = std::move(username);
  return *this;
}

NetworkProxySettings& NetworkProxySettings::WithPassword(std::string password) {
  password_ = std::move(password);
  return *this;
}

}  // namespace http
}  // namespace olp
