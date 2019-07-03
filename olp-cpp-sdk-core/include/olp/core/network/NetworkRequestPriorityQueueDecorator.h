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
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <olp/core/CoreApi.h>
#include "NetworkProtocol.h"

namespace olp {
namespace network {
struct RequestContext;
class NetworkRequestPriorityQueue;

/**
 * @brief Decorates a NetworkProtocol a queueing mechanism.
 */
class CORE_API NetworkRequestPriorityQueueDecorator : public NetworkProtocol {
 public:
  /**
   * @brief Decorates a protocol with a priority queue
   * @param protocol protocol to be decorated
   * @param default_quota_group_size defines the quota for the default group
   * @param queue_configs configurations for additional queues, consists of a
   *                      quota and a selection criterion
   */
  explicit NetworkRequestPriorityQueueDecorator(
      std::shared_ptr<NetworkProtocol> protocol,
      std::size_t default_quota_group_size =
          (std::numeric_limits<std::size_t>::max)(),
      const std::vector<std::pair<std::size_t, RequestFilter> >& queue_configs =
          {});

  /**
   * @brief Destructor.
   */
  ~NetworkRequestPriorityQueueDecorator() override;

  /**
   * @brief initialize the protocol
   * @return true if succeeded
   */
  bool Initialize() override;

  /**
   * @brief deinitialize the protocol
   */
  void Deinitialize() override;

  /**
   * @brief check if protocol is initialized
   * @return true if initialized
   */
  bool Initialized() const override;

  /**
   * @brief check if protocol can send
   * @return true if protocol can send
   */
  bool Ready() override;

  /**
   * @brief send request using the protocol
   * @param request - request to be send
   * @param id - unique request id
   * @param payload - stream to store response payload to
   * @param config - configuration for the request
   * @param headerCallback - callback to be called when a header is received
   * @param dataCallback - callback to be called when a block of data is
   * received
   * @param callback - callback to be called when the request is processed
   * @return one of common error codes
   */
  ErrorCode Send(const NetworkRequest& request, int id,
                 const std::shared_ptr<std::ostream>& payload,
                 std::shared_ptr<NetworkConfig> config,
                 Network::HeaderCallback headerCallback,
                 Network::DataCallback dataCallback,
                 Network::Callback callback) override;

  /**
   * @brief cancel the request if possible
   * @param id - unique request id
   * @return true if request was cancelled, else false
   */
  bool Cancel(int id) override;

  /**
   * @brief cancel the request if pending in queue
   * @param id - unique request id
   * @return true if request was cancelled, else false
   */
  bool CancelIfPending(int id) override;

  /**
   * @brief get amount of pending requests
   * @return amount of requests
   */
  size_t AmountPending() override;

 private:
  using QuotaCounter = std::shared_ptr<std::atomic<std::size_t> >;

  void AddDefaultQuotaGroup(std::size_t default_quota_group_size);
  void AddQuotaGroup(std::size_t quota, RequestFilter request_filter);
  void SendRequest(const std::shared_ptr<NetworkRequestPriorityQueue>& queue,
                   const std::shared_ptr<RequestContext>& request_ptr,
                   const QuotaCounter& quota_counter);

  struct QuotaQueue {
    QuotaQueue(std::shared_ptr<NetworkRequestPriorityQueue> q, std::size_t s,
               RequestFilter sel, QuotaCounter c)
        : queue(q), quota(s), selector(sel), counter(c) {}
    std::shared_ptr<NetworkRequestPriorityQueue> queue;
    std::size_t quota;
    RequestFilter selector;
    QuotaCounter counter;
  };

  class Signal {
   public:
    void Set() {
      std::lock_guard<std::mutex> lock(mutex_);
      is_satisfied_ = true;
      condition_.notify_one();
    }

    void Wait() {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.wait(lock, [this] { return is_satisfied_; });
      is_satisfied_ = false;
    }

   private:
    std::condition_variable condition_;
    bool is_satisfied_ = false;
    std::mutex mutex_;
  };

  std::shared_ptr<NetworkProtocol> protocol_;
  std::vector<QuotaQueue> queues_;
  std::shared_ptr<Signal> signal_;
  std::thread thread_;
  std::atomic<bool> stop_thread_;
  std::mutex cancel_mutex_;
};

}  // namespace network
}  // namespace olp
