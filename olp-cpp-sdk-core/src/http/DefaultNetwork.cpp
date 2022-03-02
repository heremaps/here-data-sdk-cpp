/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <algorithm>

#include "DefaultNetwork.h"
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/http/NetworkUtils.h"

#include <iostream>
#include <thread>

namespace olp {
namespace http {
DefaultNetwork::DefaultNetwork(std::shared_ptr<Network> network)
    : current_statistics_bucket_{0}, network_{std::move(network)} {
  default_network_thread_ = std::thread([&]() -> void {
    while (!stopped) {
      {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]() { return stopped || !queue_.empty(); });
      }

      while (!queue_.empty() && network_->Ready()) {
        auto data = queue_.front();

        auto queue_response = network_->Send(
            std::move(data.request), std::move(data.payload),
            std::move(data.user_callback), std::move(data.header_callback),
            std::move(data.data_callback));

        if (queue_response.IsSuccessful()) {
          std::lock_guard<std::mutex> lg(mutex);
          queue_.pop();
          continue;
        }
        queue_.pop();
        std::cout << "queue_response: "
                  << static_cast<int>(queue_response.GetErrorCode())
                  << std::endl;
        data.user_callback(NetworkResponse().WithStatus(403));
      }
    }
  });
}

DefaultNetwork::~DefaultNetwork() {
  std::cout << "Destructor" << std::endl;
  stopped = true;
  cv.notify_one();

  if (default_network_thread_.joinable()) {
    default_network_thread_.join();
  } else {
    default_network_thread_.detach();
  }
}

SendOutcome DefaultNetwork::Send(NetworkRequest request, Payload payload,
                                 Callback callback,
                                 HeaderCallback header_callback,
                                 DataCallback data_callback) {
  {
    //        std::cout << "CURL network send 0 begin: " <<
    //        std::this_thread::get_id() << ", " << std::endl;

    auto& request_headers = request.GetMutableHeaders();

    //    std::cout << "Network: " << std::this_thread::get_id() << std::endl;

    std::unique_lock<std::mutex> lock(default_headers_mutex_);
    AppendUserAgent(request_headers);
    AppendDefaultHeaders(request_headers);
  }

  const auto bucket_id = current_statistics_bucket_.load();

  std::function<void(NetworkResponse)> user_callback =
      [=](NetworkResponse response) {
        LockStatistics(bucket_id, [&](Statistics& stats) {
          const auto status = response.GetStatus();
          if (status < HttpStatusCode::OK ||
              status >= HttpStatusCode::BAD_REQUEST) {
            stats.total_failed++;
          }

          stats.total_requests++;
          stats.bytes_downloaded += response.GetBytesDownloaded();
          stats.bytes_uploaded += response.GetBytesUploaded();
        });

        if (callback) {
          callback(std::move(response));
        }
      };

  ++counter;
  if (!network_->IsStarted()) {
    return network_->Send(std::move(request), std::move(payload),
                          std::move(user_callback), std::move(header_callback),
                          std::move(data_callback));
  }

  NetRequest net_request;
  net_request.data_callback = std::move(data_callback);
  net_request.header_callback = std::move(header_callback);
  net_request.request = std::move(request);
  net_request.payload = std::move(payload);
  net_request.user_callback = std::move(user_callback);
  {
    std::lock_guard<std::mutex> lg(mutex);
    queue_.push(net_request);
    cv.notify_one();
  }

  return SendOutcome(counter);
}

void DefaultNetwork::Cancel(RequestId id) { network_->Cancel(id); }

void DefaultNetwork::SetDefaultHeaders(Headers headers) {
  std::unique_lock<std::mutex> lock(default_headers_mutex_);
  default_headers_ = std::move(headers);
  user_agent_ = NetworkUtils::ExtractUserAgent(default_headers_);
}

void DefaultNetwork::SetCurrentBucket(uint8_t bucket_id) {
  current_statistics_bucket_.store(bucket_id);
}

DefaultNetwork::Statistics DefaultNetwork::GetStatistics(uint8_t bucket_id) {
  Statistics result;
  LockStatistics(bucket_id,
                 [&](Statistics& statistics) { result = statistics; });
  return result;
}

void DefaultNetwork::AppendUserAgent(Headers& request_headers) const {
  if (user_agent_.empty()) {
    return;
  }
  auto user_agent_it =
      std::find_if(std::begin(request_headers), std::end(request_headers),
                   [](const Header& header) {
                     return NetworkUtils::CaseInsensitiveCompare(
                         header.first, kUserAgentHeader);
                   });

  if (user_agent_it != std::end(request_headers)) {
    user_agent_it->second.append(" ").append(user_agent_);
  } else {
    request_headers.emplace_back(kUserAgentHeader, user_agent_);
  }
}

void DefaultNetwork::AppendDefaultHeaders(Headers& request_headers) const {
  request_headers.insert(request_headers.end(), default_headers_.begin(),
                         default_headers_.end());
}

void DefaultNetwork::LockStatistics(uint8_t bucket_id,
                                    std::function<void(Statistics&)> callback) {
  buckets_.locked(
      [&](BucketsContainer& container) { callback(container[bucket_id]); });
}

}  // namespace http
}  // namespace olp
