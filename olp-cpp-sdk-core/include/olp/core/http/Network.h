/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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
#include <string>

#include "olp/core/CoreApi.h"
#include "olp/core/http/NetworkInitializationSettings.h"
#include "olp/core/http/NetworkRequest.h"
#include "olp/core/http/NetworkResponse.h"
#include "olp/core/http/NetworkTypes.h"

namespace olp {
/// Provides a platform specific network abstraction layer.
namespace http {

/// An HTTP client abstraction.
class CORE_API Network {
 public:
  /// The callback that is called when the request is processed or canceled.
  using Callback = std::function<void(NetworkResponse response)>;

  /// The callback that is called when a header is received.
  using HeaderCallback =
      std::function<void(std::string key, std::string value)>;

  /// The callback that is called when a chunk of data is received.
  using DataCallback = std::function<void(
      const std::uint8_t* data, std::uint64_t offset, std::size_t length)>;

  /// The request and response payload type.
  using Payload = std::shared_ptr<std::ostream>;

  /// Network statistics for a specific bucket.
  struct Statistics {
    /// The total bytes downloaded, including the size of headers and payload.
    uint64_t bytes_downloaded{0ull};

    /// The total bytes uploaded, including the size of headers and payload.
    uint64_t bytes_uploaded{0ull};

    /// The total number of requests made by network.
    uint32_t total_requests{0u};

    /// The total number of requests that failed.
    uint32_t total_failed{0u};
  };

  virtual ~Network() = default;

  /**
   * @brief Sends the network request.
   *
   * @param[in] request The request that is sent.
   * @param[in] payload The stream used to store the response payload data.
   * @param[in] callback The callback that is called when the request is fully
   * processed or canceled. After this call, no more callbacks are triggered,
   * and you can consider the request as done.
   * @param[in] header_callback The callback that is called when an HTTP header
   * is received. Each HTTP header entry results in a callback.
   * @param[in] data_callback The callback that is called when a chunk of data
   * is received. You can trigger triggered multiple times before the final
   * `Callback` call.
   *
   * @return `SendOutcome` that represents either a valid `RequestId` as
   * the unique request identifier or an `ErrorCode` in case of failure.
   * In case of failure, no callbacks are triggered.
   */
  virtual SendOutcome Send(NetworkRequest request, Payload payload,
                           Callback callback,
                           HeaderCallback header_callback = nullptr,
                           DataCallback data_callback = nullptr) = 0;

  /**
   * @brief Cancels the request associated with the given `RequestId`.
   *
   * When the request is canceled, the user receives a final callback with
   * an appropriate `NetworkResponse` marked as canceled as illustrated in the
   * following code snippet:
   *
   * @code
   *
   * auto response =
   *     http::NetworkResponse()
   *         .WithRequestId(id)
   *         .WithBytesDownloaded(download_bytes)
   *         .WithBytesUploaded(upload_bytes)
   *         .WithStatus(static_cast<int>(http::ErrorCode::CANCELLED_ERROR))
   *         .WithError("Cancelled");
   *
   * @endcode
   *
   * @note If the provided `RequestId` does not match any pending requests, no
   * operations will be performed, and no callbacks will be called.
   *
   * @param[in] id The unique ID of the request that you want to cancel.
   */
  virtual void Cancel(RequestId id) = 0;

  /**
   * @brief Sets the default network headers.
   *
   * Default headers are applied to each request passed to the `Send` method.
   * User agents are concatenated.
   *
   * @param headers The default headers.
   */
  virtual void SetDefaultHeaders(Headers headers);

  /**
   * @brief Sets the current bucket statistics.
   *
   * @param[in] bucket_id The bucket ID.
   */
  virtual void SetCurrentBucket(uint8_t bucket_id);

  /**
   * @brief Gets the statistics for a bucket.
   *
   * By default, it returns the statistics for the default bucket and the ID
   * that equals 0.
   *
   * @param[in] bucket_id The bucket ID.
   *
   * @return The statistic for the requested bucket.
   */
  virtual Statistics GetStatistics(uint8_t bucket_id = 0);
};

/// Creates a default `Network` implementation.
CORE_API std::shared_ptr<Network> CreateDefaultNetwork(
    NetworkInitializationSettings settings);

}  // namespace http
}  // namespace olp
