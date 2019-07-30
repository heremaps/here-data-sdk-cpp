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

#include "Network.h"

#include <memory>

namespace olp {
namespace network2 {

namespace {
class NetworkCurlImpl;
}

class NetworkCurl : public Network {
 public:
  NetworkCurl();

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
  RequestId Send(NetworkRequest request, std::shared_ptr<std::ostream> payload,
                 Callback callback, HeaderCallback header_callback = nullptr,
                 DataCallback data_callback = nullptr) noexcept override;

  /**
   * @brief Cancel request by id.
   * @param[in] id Request id.
   */
  void Cancel(RequestId id) noexcept override;

 private:
  std::shared_ptr<NetworkCurlImpl> impl_;
};

}  // namespace network2
}  // namespace olp
