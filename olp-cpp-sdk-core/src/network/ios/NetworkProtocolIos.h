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

#include <olp/core/network/NetworkProtocol.h>
#include <memory>

namespace olp {
namespace network {
class NetworkProtocolIosImpl;

/**
 * @brief The NetworkProtocolIos class is implementation of NetworkProtocol on
 * iOS platform see NetworkProtocol::Implementation
 */
class NetworkProtocolIos : public NetworkProtocol {
 public:
  /**
   * @brief NetworkProtocolIos constructor
   */
  NetworkProtocolIos();

  /**
   * @brief NetworkProtocolIos destructor
   */
  ~NetworkProtocolIos();

  bool Initialize() override;

  void Deinitialize() override;

  bool Initialized() const override;

  bool Ready() override;

  NetworkProtocol::ErrorCode Send(const NetworkRequest& request, int identifier,
                                  const std::shared_ptr<std::ostream>& payload,
                                  std::shared_ptr<NetworkConfig> config,
                                  Network::HeaderCallback headerCallback,
                                  Network::DataCallback dataCallback,
                                  Network::Callback callback) override;

  bool Cancel(int id) override;

  size_t AmountPending() override;

 private:
  NetworkProtocolIosImpl* m_impl;
};
}  // network
}  // olp
