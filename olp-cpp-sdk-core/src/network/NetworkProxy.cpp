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

#include "olp/core/network/NetworkProxy.h"

#include <string>

namespace olp {
namespace network {
NetworkProxy::NetworkProxy(const std::string& name, std::uint16_t port,
                           Type type, const std::string& user_name,
                           const std::string& user_password)
    : _name(name),
      port_(port),
      type_(type),
      user_name_(user_name),
      user_password_(user_password) {}

bool NetworkProxy::IsValid() const {
  return type_ != Type::Unknown && !_name.empty();
}

const std::string& NetworkProxy::Name() const { return _name; }

std::uint16_t NetworkProxy::Port() const { return port_; }

NetworkProxy::Type NetworkProxy::ProxyType() const { return type_; }

const std::string& NetworkProxy::UserName() const { return user_name_; }

const std::string& NetworkProxy::UserPassword() const { return user_password_; }

}  // namespace network
}  // namespace olp
