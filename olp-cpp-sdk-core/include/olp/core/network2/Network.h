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

#include <cstdint>
#include <functional>

#include <olp/core/CoreApi.h>
#include <olp/core/network2/NetworkRequest.h>
#include <olp/core/network2/NetworkResponse.h>
#include <olp/core/network2/NetworkTypes.h>

namespace olp {
namespace network2 {
/**
 * @brief The Network class represents HTTP client abstraction.
 */
class CORE_API Network {
 public:
  /// Represents callback to be called when the request has been processed or
  /// cancelled.
  using Callback = std::function<void(NetworkResponse response)>;

  /// Represents callback to be called when a header has been received.
  using HeaderCallback =
      std::function<void(std::string key, std::string value)>;

  /// Represents callback to be called when a chunk of data has been received.
  using DataCallback = std::function<void(const uint8_t* data,
                                          std::uint64_t offset, size_t len)>;

  virtual ~Network();

  /**
   * @brief Send network request.
   * @param[in] request Request to be sent.
   * @param[in] payload Stream to store response payload data.
   * @param[in] callback Callback to be called when request is processed or
   * cancelled.
   * @param[in] header_callback Callback to be called when a header is
   * received.
   * @param[in] data_callback Callback to be called when a chunk of data is
   * received.
   * @return requests id assigned to the request or
   * RequestIdConstants::RequestIdInvalid in case of failure.
   */
  virtual RequestId Send(NetworkRequest request,
                         std::shared_ptr<std::ostream> payload,
                         Callback callback,
                         HeaderCallback header_callback = nullptr,
                         DataCallback data_callback = nullptr) = 0;

  /**
   * @brief Cancel request by id.
   * @param[in] id Request id.
   */
  virtual void Cancel(RequestId id) = 0;
};

}  // namespace network2
}  // namespace olp
