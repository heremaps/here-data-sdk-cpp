/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
/**
 * @brief Flushes requests into a HERE platform stream layer.
 */
class DATASERVICE_WRITE_API FlushRequest {
 public:
  FlushRequest() = default;

  /**
   * @return The number of \c PublishDataRequest requests, which will be
   * flushed by \c StreamLayerCLient. By default this value is initilized to 0,
   * thus, all queued \c StreamLayerClient's requests will be flushed.
   */
  inline int GetNumberOfRequestsToFlush() const {
    return num_requests_per_flush_;
  }

  /**
   * @param num_requests Maximum number of partitions (i.e. \c
   * PublishDataRequest) to be flushed by \c StreamLayerClient. If this value is
   * negative, nothing will be flushed. Set 0 to flush all requests.
   * @note Required.
   */
  inline FlushRequest& WithNumberOfRequestsToFlush(int num_requests) {
    num_requests_per_flush_ = num_requests;
    return *this;
  }

 private:
  /// Number of requests to flush
  int num_requests_per_flush_ = 0;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
