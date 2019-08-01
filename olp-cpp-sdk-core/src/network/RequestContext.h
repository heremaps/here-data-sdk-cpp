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

#include <chrono>
#include <string>

#include "Memory.h"
#include "olp/core/logging/Log.h"
#include "olp/core/network/Network.h"
#include "olp/core/network/NetworkRequest.h"

namespace olp {
namespace network {
/**
 * @brief Request Context structure
 */
struct RequestContext {
  /// Constructor
  RequestContext(const NetworkRequest& request, Network::RequestId id,
                 const Network::Callback& callback,
                 Network::HeaderCallback header_callback,
                 Network::DataCallback data_callback,
                 const std::shared_ptr<std::ostream>& payload,
                 const std::shared_ptr<NetworkConfig>& config)
      : request_(request),
        payload_(payload),
        callback_(callback),
        header_callback_(std::move(header_callback)),
        data_callback_(std::move(data_callback)),
        id_(id),
        config_(config) {}

  /// Destructor
  ~RequestContext() {
#if defined(EDGE_SDK_LOGGING_DISABLED)
    using namespace std::chrono;
    const auto life_time =
        duration_cast<seconds>(steady_clock::now() - creation_time_).count();

    EDGE_SDK_LOG_TRACE("RequestContext", ("Destroying context for request " +
                                          std::to_string(id_) + " after " +
                                          std::to_string(life_time) + " sec.")
                                             .c_str());
#endif
  }

  /// HTTP network request
  NetworkRequest request_;
  /// Payload
  std::shared_ptr<std::ostream> payload_;
  /// Callback
  Network::Callback callback_;
  /// Header callback
  Network::HeaderCallback header_callback_;
  /// Data callback
  Network::DataCallback data_callback_;
  /// Request id
  Network::RequestId id_;
  /// Configuration
  std::shared_ptr<NetworkConfig> config_;
  /// Memory tracking to use for the request.
  mem::MemoryScopeTracker tracker_;
  /// Time of context creation
  std::chrono::steady_clock::time_point creation_time_ =
      std::chrono::steady_clock::now();
};

using RequestContextPtr = std::shared_ptr<RequestContext>;

}  // namespace network
}  // namespace olp
