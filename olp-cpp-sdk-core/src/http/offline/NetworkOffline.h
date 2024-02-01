/*
 * Copyright (C) 2024 HERE Europe B.V.
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

#include "olp/core/http/Network.h"
#include "olp/core/http/NetworkRequest.h"

namespace olp {
namespace http {

/**
 * @brief The implementation of Network based on cURL.
 */
class NetworkOffline : public olp::http::Network,
                       public std::enable_shared_from_this<NetworkOffline> {
 public:
  /**
   * @brief NetworkOffline constructor.
   */
  NetworkOffline();

  /**
   * @brief ~NetworkOffline destructor.
   */
  ~NetworkOffline() override;

  /**
   * @brief Not copyable.
   */
  NetworkOffline(const NetworkOffline& other) = delete;

  /**
   * @brief Not movable.
   */
  NetworkOffline(NetworkOffline&& other) = delete;

  /**
   * @brief Not copy-assignable.
   */
  NetworkOffline& operator=(const NetworkOffline& other) = delete;

  /**
   * @brief Not move-assignable.
   */
  NetworkOffline& operator=(NetworkOffline&& other) = delete;

  /**
   * @brief Implementation of Send method from Network abstract class.
   */
  SendOutcome Send(NetworkRequest request, Payload payload, Callback callback,
                   HeaderCallback header_callback = nullptr,
                   DataCallback data_callback = nullptr) override;

  /**
   * @brief Implementation of Cancel method from Network abstract class.
   */
  void Cancel(RequestId id) override;
};

}  // namespace http
}  // namespace olp
