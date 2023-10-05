/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/// Flushes requests in a stream layer.
class DATASERVICE_WRITE_API FlushRequest {
 public:
  FlushRequest() = default;
  FlushRequest(const FlushRequest&) = default;
  FlushRequest(FlushRequest&&) = default;
  FlushRequest& operator=(const FlushRequest&) = default;
  FlushRequest& operator=(FlushRequest&&) = default;
  virtual ~FlushRequest() = default;

  /**
   * @brief Gets the number of partitions (`PublishDataRequest`) to be flushed.
   *
   * The default value is 0, which means that all queued
   * `StreamLayerClient` requests are flushed.
   *
   * @return The number of publish data requests.
   */
  inline int GetNumberOfRequestsToFlush() const {
    return num_requests_per_flush_;
  }

  /**
   * @brief Sets the number of partitions (`PublishDataRequest`) to be flushed.
   *
   * @param num_requests The maximum number of partitions be flushed by
   * `StreamLayerClient`. If the value is negative, nothing is flushed.
   * To flush all requests, set the value to 0.
   */
  inline FlushRequest& WithNumberOfRequestsToFlush(int num_requests) {
    num_requests_per_flush_ = num_requests;
    return *this;
  }

 private:
  /// The number of requests to flush.
  int num_requests_per_flush_ = 0;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
