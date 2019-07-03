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

#include "NetworkProtocolIos.h"
#include "NetworkProtocolIosImpl.h"

#include <memory>

namespace olp {
namespace network {
NetworkProtocolIos::NetworkProtocolIos() : m_impl(nullptr) {}

NetworkProtocolIos::~NetworkProtocolIos() {
  if (m_impl) {
    delete m_impl;
    m_impl = nullptr;
  }
}

bool NetworkProtocolIos::Initialize() {
  if (!m_impl) {
    m_impl = new NetworkProtocolIosImpl();
  }
  m_impl->Initialize();
  return true;
}

void NetworkProtocolIos::Deinitialize() {
  if (m_impl) {
    m_impl->Deinitialize();
  }
}

bool NetworkProtocolIos::Initialized() const {
  if (m_impl) {
    return m_impl->Initialized();
  }
  return false;
}

bool NetworkProtocolIos::Ready() {
  if (m_impl) {
    return m_impl->Ready();
  }
  return false;
}

size_t NetworkProtocolIos::AmountPending() {
  if (m_impl) {
    return m_impl->AmountPending();
  }
  return 0;
}

NetworkProtocol::ErrorCode NetworkProtocolIos::Send(
    const NetworkRequest& request, int identifier,
    const std::shared_ptr<std::ostream>& payload,
    std::shared_ptr<NetworkConfig> config,
    Network::HeaderCallback headerCallback, Network::DataCallback dataCallback,
    Network::Callback callback) {
  if (m_impl) {
    return m_impl->Send(request, identifier, payload, config, headerCallback,
                        dataCallback, callback);
  }

  return ErrorNoConnection;
}

bool NetworkProtocolIos::Cancel(int id) {
  if (m_impl) {
    return m_impl->Cancel(id);
  }
  return false;
}
}  // network
}  // olp
