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

#include <atomic>
#include <climits>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

#include "NetworkConfig.h"
#include "NetworkEvent.h"
#include "NetworkRequest.h"
#include "NetworkSystemConfig.h"
#include "olp/core/thread/Atomic.h"

namespace olp {
namespace network {
class NetworkResponse;
class NetworkSingleton;

using ClientIdType = unsigned;

enum class ClientId : ClientIdType { Invalid = 0, Min = 1 };

/**
 * @brief The Network class represents HTTP client abstraction
 */
class CORE_API Network {
 public:
  /**
   * @brief List of special values for NetworkRequestId
   */
  typedef enum {
    NetworkRequestIdInvalid = INT_MIN,
    NetworkRequestIdMin = INT_MIN + 1,
    NetworkRequestIdMax = INT_MAX
  } NetworkRequestIdConstants;

  /**
   * @brief NetworkRequestId represents request id
   * Values of this type are returned by Network::send and used by
   * Network::cancel. This value becomes invalid after Network::NetworkCallback
   * is called.
   */
  typedef int RequestId;

  /**
   * @brief Error codes
   */
  enum ErrorCode {
    IOError = -1,
    AuthorizationError = -2,
    InvalidURLError = -3,
    Offline = -4,
    Cancelled = -5,
    AuthenticationError = -6,
    TimedOut = -7,
    UnknownError = -8
  };

  /**
   * @brief ConnectionStatus provides list of possible network connectivity
   * statuses.
   *
   * ShuttingDown Used when last existing Network instance is being destroyed.
   * All callbacks will be unregistered.
   */
  enum class ConnectionStatus {
    Valid,
    NoConnection,
    ConnectionReestablished,
    ShuttingDown
  };

  /**
   * @brief NetworkCallback represents callback to be called when request
   * processed or cancelled
   */
  using Callback = std::function<void(const NetworkResponse& response)>;

  /**
   * @brief HeaderCallback represents callback to be called when a header has
   * been received
   */
  using HeaderCallback =
      std::function<void(const std::string& key, const std::string& value)>;

  /**
   * @brief DataCallback represents callback to be called when a chunk of data
   * has been received
   */
  using DataCallback = std::function<void(std::uint64_t offset,
                                          const uint8_t* data, size_t len)>;

  /**
   * @brief NetworkStatusChangedCallback informs listeners about changes in
   * connectivity
   */
  using NetworkStatusChangedCallback =
      std::function<void(const ConnectionStatus& networkStatus)>;

  /**
   * @brief Network constructor
   */
  Network();

  /**
   * @brief Network destructor
   * This method stops the network client and clears all queues
   */
  virtual ~Network();

  /**
   * @brief Network is not copyable
   */
  Network(const Network&) = delete;

  /**
   * @brief Network is not moveable
   */
  Network(Network&&) = delete;

  /**
   * @brief Network is not copy-assignable
   */
  Network& operator=(const Network&) = delete;

  /**
   * @brief Network is not move-assignable
   */
  Network& operator=(Network&&) = delete;

  /**
   * @brief Start the network client
   * @param config - network configuration
   * @return true if succeeded and false otherwise
   */
  virtual bool Start(const NetworkConfig& config = NetworkConfig());

  /**
   * @brief Stop the network client
   * @return true if succeeded and false otherwise
   */
  virtual bool Stop();

  /**
   * @brief Convenient method to restart the Network client.
   * @param config - network configuration
   * @return true if succeeded and false otherwise
   */
  virtual bool Restart(const NetworkConfig& config = NetworkConfig());

  /**
   * @brief Check whether the network client is started or not
   * @return true if server started and false otherwise
   */
  virtual bool Started() const;

  /**
   * @brief Asynchronously send a network request
   * @param request - request to be sent
   * @param payload - stream to store response payload data
   * @param callback - callback to be called when request is processed or
   * cancelled
   * @param header_callback - callback to be called when a header is received
   * @param data_callback - callback to be called when a block of data is
   * received
   * @return network request id assigned to the request or
   * NetworkRequestIdInvalid if fails
   */
  virtual RequestId Send(NetworkRequest request,
                         const std::shared_ptr<std::ostream>& payload,
                         const Callback& callback,
                         const HeaderCallback& header_callback = nullptr,
                         const DataCallback& data_callback = nullptr);

  /**
   * @brief Send a network request and wait for the response before returning.
   * @param request - request to be sent
   * @param payload - stream to store the response payload data
   * @param headers - response headers
   * @return network response
   */
  virtual NetworkResponse SendAndWait(
      NetworkRequest request, const std::shared_ptr<std::ostream>& payload,
      std::vector<std::pair<std::string, std::string> >& headers);

  /**
   * @brief Cancel a network request
   * @param id - network request id returned by send
   * @return true if cancellation was initiated and false otherwise
   */
  virtual bool Cancel(RequestId id);

  /**
   * @brief Cancel a request if it hasn't started yet
   * @param id - network request id returned by send
   * @return true if cancellation was initiated and false otherwise
   */
  virtual bool CancelIfPending(RequestId id);

  /**
   * @brief resets system wide network configuration
   */
  static void ResetSystemConfig();

  /**
   * @brief Return network system wide configuration
   * @return reference to network configuration object - atomically modifiable
   */
  static thread::Atomic<NetworkSystemConfig>& SystemConfig();

  /**
   * @brief Adds listener to Network; works across different instances of
   * network. Callback is called when connection status is changed
   * (lost/reestablished)
   * @param [in] callback
   * @return id which can be used to remove listener callback. Valid range
   * starts from 1. 0 signifies error.
   */
  static std::int32_t AddListenerCallback(
      const NetworkStatusChangedCallback& callback);

  /**
   * @brief Removes listener from Network; works across different instances of
   * network.
   * @param [in] callback_id Id of registered callback (see
   * addListenerCallback). Valid range starts from 1.
   */
  static void RemoveListenerCallback(std::int32_t callback_id);

  /**
   * @brief Set certificate updater to Network.
   * @param [in] update_certificate updating function
   */
  static void SetCertificateUpdater(std::function<void()> update_certificate);

 public:
  /**
   * @brief Statistics structure
   */
  struct Statistics {
    /// Number of requests completed
    std::uint64_t requests_;
    /// Number of failures
    std::uint64_t errors_;
    /// Amount of data received as content
    std::uint64_t content_bytes_;
  };

  /**
   * @brief Retrieve current statistics
   * @param [out] statistics as current completed http statistics
   * @return true if the statistics are available
   */
  static bool GetStatistics(Statistics& statistics);

  /**
   * @brief Unblock network
   */
  static void Unblock();

  /// A container of request ids
  struct RequestIds {
    /**
     * @brief insert
     * @param id id
     */
    void Insert(Network::RequestId id);

    /**
     * @brief remove
     * @param id request id
     */
    void Remove(Network::RequestId id);

    /**
     * @brief Clear stored ids and callbacks
     * @return Removed ids and callbacks
     */
    std::vector<Network::RequestId> Clear();

   private:
    std::mutex mutex_;
    std::vector<Network::RequestId> data_;
  };

 private:
  std::shared_ptr<NetworkSingleton> GetSingleton() const;

 private:
  mutable std::mutex mutex_;
  std::shared_ptr<NetworkSingleton> singleton_;
  ClientId id_;
  std::shared_ptr<NetworkConfig> config_;
  std::shared_ptr<RequestIds> request_ids_ = std::make_shared<RequestIds>();
};

}  // namespace network
}  // namespace olp
