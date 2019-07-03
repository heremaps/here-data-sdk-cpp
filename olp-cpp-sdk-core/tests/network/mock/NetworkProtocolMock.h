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

#include <olp/core/network/NetworkFactory.h>
#include <olp/core/network/NetworkProtocol.h>

#include <gmock/gmock.h>
#include <memory>

namespace olp {
namespace network {
namespace test {
/**
 * @brief The NetworkProtocolMock class is a mock implementation to be used in
 * tests
 */
class NetworkProtocolMock : public NetworkProtocol {
 public:
  MOCK_METHOD0(Initialize, bool());

  MOCK_METHOD0(Deinitialize, void());

  MOCK_CONST_METHOD0(Initialized, bool());

  MOCK_METHOD0(Ready, bool());

  MOCK_METHOD7(Send, ErrorCode(const NetworkRequest& /*request*/, int /*id*/,
                               const std::shared_ptr<std::ostream>& /*payload*/,
                               std::shared_ptr<NetworkConfig> /*config*/,
                               Network::HeaderCallback /*headerCallback*/,
                               Network::DataCallback /*dataCallback*/,
                               olp::network::Network::Callback /*callback*/));

  MOCK_METHOD1(Cancel, bool(int));

  MOCK_METHOD1(CancelIfPending, bool(int));

  MOCK_METHOD0(AmountPending, size_t());
};

/**
 * @brief The NetworkProtocolMockFactory class returns the mock network protocol
 * implementation
 */
class NetworkProtocolMockFactory : public NetworkProtocolFactory {
 public:
  NetworkProtocolMockFactory()
      : m_network_protocol_mock(std::make_shared<NetworkProtocolMock>()) {}

  std::shared_ptr<NetworkProtocol> Create(void*) const override {
    return m_network_protocol_mock;
  }

 public:
  /// Member to access & configure mock expectations in tests
  std::shared_ptr<NetworkProtocolMock> m_network_protocol_mock;
};

}  // namespace test
}  // namespace network
}  // namespace olp
