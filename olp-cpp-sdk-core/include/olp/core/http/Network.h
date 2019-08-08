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
#include <memory>

#include <olp/core/CoreApi.h>
#include <olp/core/http/NetworkRequest.h>
#include <olp/core/http/NetworkResponse.h>
#include <olp/core/http/NetworkTypes.h>

namespace olp {
namespace http {

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
  using DataCallback = std::function<void(
      const std::uint8_t* data, std::uint64_t offset, std::size_t length)>;

  /// Represents the request and response payload type
  using Payload = std::shared_ptr<std::ostream>;

  virtual ~Network() = default;

  /**
   * @brief Send network request.
   * @param[in] request Request to be sent.
   * @param[in] payload Stream to store response payload data.
   * @param[in] callback Callback to be called when request is fully processed
   * or cancelled. After this call there will be no more callbacks triggered and
   * users can consider the request as done.
   * @param[in] header_callback Callback to be called when a HTTP header has
   * been received. Each HTTP header entry will result in a callback.
   * @param[in] data_callback Callback to be called when a chunk of data is
   * received. This callback can be triggered multiple times all prior to the
   * final Callback call.
   * @return SendOutcome which represent either a valid \c RequestId as the
   * unique request identifier or a \c ErrorCode in case of failure. In case of
   * failure no callbacks will be triggered.
   */
  virtual SendOutcome Send(NetworkRequest request, Payload payload,
                           Callback callback,
                           HeaderCallback header_callback = nullptr,
                           DataCallback data_callback = nullptr) = 0;

  /**
   * @brief Cancel request by RequestId.
   * Once the request was cancelled the user will receive a final Callback with
   * an appropriate NetworkResponse marked as cancelled.
   * @param[in] id The unique RequestId of the request to be cancelled.
   */
  virtual void Cancel(RequestId id) = 0;
};

}  // namespace http
}  // namespace olp
