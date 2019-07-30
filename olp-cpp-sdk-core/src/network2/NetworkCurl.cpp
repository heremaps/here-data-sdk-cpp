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

#include "olp/core/network2/NetworkCurl.h"
#include "olp/core/network2/NetworkTypes.h"

#include <atomic>

#include "olp/core/logging/Log.h"

#include <curl/curl.h>

namespace olp {
namespace network2 {

namespace {
constexpr auto kLogTag = "NetworkCurl";

class NetworkCurlImpl final {
 public:
  RequestId Send(NetworkRequest request,
                 std::shared_ptr<std::ostream> /*payload*/,
                 Network::Callback /*callback*/,
                 Network::HeaderCallback /*header_callback*/,
                 Network::DataCallback /*data_callback*/) noexcept {
    RequestId currentId = id_counter_++;
#ifdef NETWORK_REQUESTID_OVERFLOW_HANDLING
    // may be needed if RequestIdConstants defines a short range of valid
    // IDs
    if (currentId >= RequestIdConstants::RequestIdMax) {
      currentId = RequestIdConstants::RequestIdMin;
      id_counter_ = currentId + 1;
    }
#endif // NETWORK_REQUESTID_OVERFLOW_HANDLING

    LOG_INFO_F(kLogTag, "Send to %s, method: %d, id: %ld",
               request.GetUrl().c_str(), request.GetVerb(), currentId);

    return currentId;
  }

  void Cancel(RequestId /*id*/) noexcept {}

 private:
  std::atomic<RequestId> id_counter_{RequestIdConstants::RequestIdMin};
};
}  // namespace

NetworkCurl::NetworkCurl()
    : impl_(std::make_shared<olp::network2::NetworkCurlImpl>()) {}

RequestId NetworkCurl::Send(NetworkRequest request,
                            std::shared_ptr<std::ostream> payload,
                            Network::Callback callback,
                            Network::HeaderCallback header_callback,
                            Network::DataCallback data_callback) noexcept {
  LOG_TRACE(kLogTag, "Send");
  return impl_->Send(request, payload, callback, header_callback,
                     data_callback);
}

void NetworkCurl::Cancel(RequestId id) noexcept {
  LOG_TRACE(kLogTag, "Cancel");
  impl_->Cancel(id);
}

}  // namespace network2
}  // namespace olp
