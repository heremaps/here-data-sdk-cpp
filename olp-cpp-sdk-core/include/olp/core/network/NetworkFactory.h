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

#include <memory>
#include <mutex>

#include "Network.h"
#include "NetworkProtocol.h"

#if defined(__ANDROID__) && !defined(NETWORK_ANDROID)
#define NETWORK_ANDROID
#endif

namespace olp {
namespace network {
/**
 * @brief The NetworkFactory class creates the network protocol.
 */
class CORE_API NetworkFactory {
 public:
  /// set the network protocol factory pointer
  static void SetNetworkProtocolFactory(
      std::shared_ptr<NetworkProtocolFactory> factory);

  /// returns a new network protocol instance
  static std::shared_ptr<NetworkProtocol> CreateNetworkProtocol();
};

}  // namespace network
}  // namespace olp
