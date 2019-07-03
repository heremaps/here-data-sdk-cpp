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

#include "olp/core/network/NetworkRequestPriorityQueueDecorator.h"

#include <algorithm>
#include <cassert>
#include <future>
#include <limits>
#include <string>
#include <tuple>

#include "Memory.h"
#include "NetworkEventImpl.h"
#include "NetworkRequestPriorityQueue.h"
#include "olp/core/network/NetworkResponse.h"
#include "olp/core/porting/make_unique.h"

namespace olp {
namespace network {
namespace {
std::shared_ptr<NetworkEvent> createNetworkEvent(const std::string& url) {
  return std::make_shared<NetworkEventImpl>(url);
}
}  // namespace

NetworkRequestPriorityQueueDecorator::NetworkRequestPriorityQueueDecorator(
    std::shared_ptr<NetworkProtocol> protocol,
    std::size_t default_quota_group_size,
    const std::vector<std::pair<std::size_t, RequestFilter> >& queue_configs)
    : protocol_(std::move(protocol)),
      signal_(std::make_shared<Signal>()),
      stop_thread_{false} {
  AddDefaultQuotaGroup(default_quota_group_size);
  for (const auto& config : queue_configs) {
    AddQuotaGroup(config.first, config.second);
  };

  std::promise<void> thread_start_signal;
  thread_ = std::thread([this, &thread_start_signal] {
    thread_start_signal.set_value();
    while (!stop_thread_) {
      const auto do_wait = all_of(
          begin(queues_), end(queues_), [this](const QuotaQueue& quota_queue) {
            if (protocol_->Ready()) {
              const auto quota = quota_queue.quota;
              const auto pending_requests = quota_queue.counter;
              if (*pending_requests >= quota) {
                // quota exceeded
                return true;
              }

              const auto queue = quota_queue.queue;

              const auto request_ptr = queue->Pop();
              if (request_ptr) {
                std::unique_lock<std::mutex> lock(cancel_mutex_);
                SendRequest(queue, request_ptr, pending_requests);
                // possibly more requests in this queue
                //-> don't wait for more requests
                return false;
              }

              // no request in this queue
              return true;
            }

            // wait until protocol is ready again
            signal_->Wait();
            // during waiting new requests might be queued
            // -> don't wait after iteration over all queues
            return false;
          });

      // wait if all queues were empty on last visit and
      // the iteration over the queues did not contain a waiting period
      if (do_wait) {
        signal_->Wait();
      }
    }
  });

  // wait until thread is running
  thread_start_signal.get_future().wait();
}

NetworkRequestPriorityQueueDecorator::~NetworkRequestPriorityQueueDecorator() {
  stop_thread_ = true;
  signal_->Set();
  if (thread_.joinable()) {
    if (thread_.get_id() != std::this_thread::get_id()) {
      thread_.join();
    } else {
      thread_.detach();
    }
  }
}

bool NetworkRequestPriorityQueueDecorator::Initialize() {
  return protocol_->Initialize();
}

void NetworkRequestPriorityQueueDecorator::Deinitialize() {
  return protocol_->Deinitialize();
}

bool NetworkRequestPriorityQueueDecorator::Initialized() const {
  return protocol_->Initialized();
}

bool NetworkRequestPriorityQueueDecorator::Ready() { return true; }

NetworkProtocol::ErrorCode NetworkRequestPriorityQueueDecorator::Send(
    const NetworkRequest& request, int id,
    const std::shared_ptr<std::ostream>& payload,
    std::shared_ptr<NetworkConfig> config,
    Network::HeaderCallback headerCallback, Network::DataCallback dataCallback,
    Network::Callback callback) {
  auto context = std::make_shared<RequestContext>(
      request, id, callback, headerCallback, dataCallback, payload, config);

  // m_queues.front() needs to be visited last since it accepts all (remaining)
  // requests
  const auto quota_queue_end = queues_.rend();
  for (auto quota_queue = queues_.rbegin(); quota_queue != quota_queue_end;
       ++quota_queue) {
    const auto selector = quota_queue->selector;

    if (selector(request)) {
      const auto queue = quota_queue->queue;
      queue->Push(context);
      signal_->Set();
      return NetworkProtocol::ErrorNone;
    }
  }

  return NetworkProtocol::ErrorNetworkOverload;
}

bool NetworkRequestPriorityQueueDecorator::Cancel(int id) {
  const auto was_removed_from_queue = CancelIfPending(id);
  auto result = true;

  if (!was_removed_from_queue) {
    {
      std::lock_guard<std::mutex> lock(cancel_mutex_);
      if (!protocol_->Cancel(id)) {
        return false;
      }
    }
    signal_->Set();
  }

  return result;
}

bool NetworkRequestPriorityQueueDecorator::CancelIfPending(int id) {
  return any_of(
      begin(queues_), end(queues_), [id](const QuotaQueue& quota_queue) {
        const auto removed = quota_queue.queue->RemoveIf(
            [id](RequestContextPtr ptr) { return ptr->id_ == id; });
        using std::begin;
        using std::end;
        std::for_each(begin(removed), end(removed), [](RequestContextPtr ptr) {
          if (ptr->callback_) {
            auto response = NetworkResponse(
                ptr->id_, true, Network::Cancelled, "Cancelled", 0, -1, "", "",
                0, 0, nullptr, NetworkProtocol::StatisticsData());
            ptr->callback_(response);
          }
        });
        return !removed.empty();
      });
}

std::size_t NetworkRequestPriorityQueueDecorator::AmountPending() {
  return protocol_->AmountPending();
}

void NetworkRequestPriorityQueueDecorator::AddDefaultQuotaGroup(
    std::size_t default_quota_group_size) {
  AddQuotaGroup(default_quota_group_size,
                [](const NetworkRequest&) { return true; });
}

void NetworkRequestPriorityQueueDecorator::AddQuotaGroup(
    std::size_t quota, RequestFilter request_filter) {
  assert(request_filter);
  queues_.emplace_back(std::make_shared<NetworkRequestPriorityQueue>(), quota,
                       request_filter,
                       std::make_shared<std::atomic<std::size_t> >(0u));
}

void NetworkRequestPriorityQueueDecorator::SendRequest(
    const std::shared_ptr<NetworkRequestPriorityQueue>& queue,
    const std::shared_ptr<RequestContext>& request_ptr,
    const QuotaCounter& pending_requests) {
  MEMORY_TRACKER_SCOPE(request_ptr->tracker_);
  const auto protocol = protocol_;
  std::weak_ptr<NetworkProtocol> weakProtocol = protocol;
  const auto signal = signal_;
  const auto networkEvent = createNetworkEvent(request_ptr->request_.Url());
  (*pending_requests)++;
  const auto error_code = protocol_->Send(
      request_ptr->request_, request_ptr->id_, request_ptr->payload_,
      request_ptr->config_, request_ptr->header_callback_,
      request_ptr->data_callback_,
      [weakProtocol, queue, signal, request_ptr, pending_requests,
       networkEvent](const NetworkResponse& response) {
        if (networkEvent) {
          auto protocol = weakProtocol.lock();
          if (protocol) {
            const int queueSize =
                queue->Size() > std::numeric_limits<int>::max()
                    ? std::numeric_limits<int>::max()
                    : static_cast<int>(queue->Size());
            networkEvent->Record(response.PayloadSize(), response.Status(),
                                 request_ptr->request_.Priority(), queueSize,
                                 request_ptr->request_.Url(),
                                 request_ptr->request_.ExtraHeaders(),
                                 protocol->AmountPending());
          }
        }

        if (request_ptr->callback_) {
          request_ptr->callback_(response);
        }
        (*pending_requests)--;
        signal->Set();
      });
  if (error_code == NetworkProtocol::ErrorNotReady) {
    queue->Push(request_ptr);
  } else if (error_code != NetworkProtocol::ErrorNone) {
    MEMORY_TRACKER_SCOPE(request_ptr->tracker_);
    if (request_ptr->callback_) {
      HandleSynchronousNetworkErrors(error_code, request_ptr->id_,
                                     request_ptr->callback_);
    }
    (*pending_requests)--;
    signal->Set();
  }
}

}  // namespace network
}  // namespace olp
