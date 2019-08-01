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

#include "olp/core/network/NetworkFactory.h"

#include <memory>

#include "olp/core/logging/Log.h"
#include "olp/core/network/NetworkProtocol.h"

namespace {
std::shared_ptr<olp::network::NetworkProtocolFactory> g_protocol_factory;
std::shared_ptr<olp::network::NetworkProtocolFactory>
    g_default_network_protocol_factory;
}  // namespace

namespace olp {
namespace network {
#define LOGTAG "NetworkFactory"

void NetworkFactory::SetNetworkProtocolFactory(
    std::shared_ptr<NetworkProtocolFactory> factory) {
  g_protocol_factory = factory;
}

std::shared_ptr<NetworkProtocol> NetworkFactory::CreateNetworkProtocol() {
  auto factory = g_protocol_factory;

  if (!factory) {
    if (!g_default_network_protocol_factory) {
      g_default_network_protocol_factory =
          std::make_shared<DefaultNetworkProtocolFactory>();
    }
    EDGE_SDK_LOG_INFO(LOGTAG,
                      "createNetworkProtocol: using default protocol factory");
    factory = g_default_network_protocol_factory;
  }

  return factory->Create();
}

}  // namespace network
}  // namespace olp
