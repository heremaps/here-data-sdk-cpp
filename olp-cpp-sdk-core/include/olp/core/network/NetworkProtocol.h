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

#include <time.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Network.h"

namespace olp {
namespace network {
class NetworkRequest;
class NetworkConfig;

/**
 * @brief The NetworkProtocol class is an abstract class for protocol
 * implemetation
 */
class CORE_API NetworkProtocol {
 public:
  /**
   * @brief Common error codes
   */
  enum ErrorCode {
    ErrorNone,
    ErrorIO,
    ErrorNotReady,
    ErrorNoConnection,
    ErrorInvalidRequest,
    ErrorFailedBindInterface,
    ErrorNetworkInterfaceOptionNotImplemented,
    ErrorNetworkOverload,
    ErrorFailedSetCaCert,
    ErrorCaCertOptionNotImplemented
  };

  /**
   * @brief Data stucture to store statistics
   */
  using StatisticsData = std::vector<std::pair<std::string, std::string> >;

  /**
   * @brief ~NetworkProtocol destructor
   */
  virtual ~NetworkProtocol() {}

  /**
   * @brief initialize the protocol
   * @return true if succeeded
   */
  virtual bool Initialize() = 0;

  /**
   * @brief deinitialize the protocol
   */
  virtual void Deinitialize() = 0;

  /**
   * @brief check if protocol is initialized
   * @return true if initialized
   */
  virtual bool Initialized() const = 0;

  /**
   * @brief check if protocol can send
   * @return true if protocol can send
   */
  virtual bool Ready() = 0;

  /**
   * @brief send request using the protocol
   * @param request - request to be send
   * @param id - unique request id
   * @param payload - stream to store response payload to
   * @param config - configuration for the request
   * @param header_callback - callback to be called when a header is received
   * @param data_callback - callback to be called when a block of data is
   * received
   * @param callback - callback to be called when the request is processed
   * @return one of common error codes
   */
  virtual ErrorCode Send(const NetworkRequest& request, int id,
                         const std::shared_ptr<std::ostream>& payload,
                         std::shared_ptr<NetworkConfig> config,
                         Network::HeaderCallback header_callback,
                         Network::DataCallback data_callback,
                         Network::Callback callback) = 0;

  /**
   * @brief cancel the request if possible
   * @param id - unique request id
   * @return true if request was cancelled, else false
   */
  virtual bool Cancel(int id) = 0;

  /**
   * @brief cancel the request if pending
   * @param id - unique request id
   * @return true if request was cancelled, else false
   */
  virtual bool CancelIfPending(int id) {
    (void)id;
    return false;
  }

  /**
   * @brief get amount of bending requests
   * @return amount of requests
   */
  virtual size_t AmountPending() = 0;

  /**
   * @brief convert HTTP error to string
   * @param error - erro code
   * @return error code as a string
   */
  static std::string HttpErrorToString(int error);

  /// Default constructor defaulted
  NetworkProtocol() = default;
  /// Copy constructor removed.
  NetworkProtocol(const NetworkProtocol& other) = delete;
  /// Move constructor removed.
  NetworkProtocol(NetworkProtocol&& other) = delete;
  /// Copy operator removed.
  NetworkProtocol& operator=(const NetworkProtocol& other) = delete;
  /// Move operator removed.
  NetworkProtocol& operator=(NetworkProtocol&& other) = delete;
};

/**
 * @brief Treat synchronous errors.
 * @param error_code error code, synchronously returned from
 * NetworkProtocol::send
 * @param request_id id of the request that was tried to send
 * @param callback callback passed to the network for the request associated
 * with request_id
 */
void HandleSynchronousNetworkErrors(
    NetworkProtocol::ErrorCode error_code, Network::RequestId request_id,
    const Network::Callback& callback);

/**
 * @brief The NetworkProtocol factory for default network protocols
 */
class CORE_API NetworkProtocolFactory {
 public:
  /**
   * @brief Destroy the factory.
   */
  virtual ~NetworkProtocolFactory() {}

  /**
   * @brief Create a new network protocol
   * @param context - context pointer depending on protocol
   * @return newly created protocol
   */
  virtual std::shared_ptr<NetworkProtocol> Create(
      void* context = nullptr) const = 0;
};

/**
 * @brief The DefaultNetworkProtocolFactory class returns the default network
 * protocol
 */
class CORE_API DefaultNetworkProtocolFactory : public NetworkProtocolFactory {
 public:
  std::shared_ptr<NetworkProtocol> Create(void*) const override;
};

}  // namespace network
}  // namespace olp
