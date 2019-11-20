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

#include <olp/core/http/Network.h>

namespace olp {
namespace tests {
namespace http {

using namespace olp::http;

/*
 * Node test server is limited to http proxy. Wrapper alters ongoing requests
 * from https to http.
 */
class Http2HttpNetworkWrapper : public Network {
 public:
  SendOutcome Send(NetworkRequest request, Payload payload, Callback callback,
                   HeaderCallback header_callback = nullptr,
                   DataCallback data_callback = nullptr) override {
    auto url = request.GetUrl();
    auto pos = url.find("https");
    if (pos != std::string::npos) {
      url.replace(pos, 5, "http");
      request.WithUrl(url);
    }

    return network_->Send(std::move(request), std::move(payload),
                          std::move(callback), std::move(header_callback),
                          std::move(data_callback));
  }

  void Cancel(RequestId id) override { network_->Cancel(id); }

  Http2HttpNetworkWrapper()
      : network_{std::move(olp::http::CreateDefaultNetwork(32))} {}

 private:
  std::shared_ptr<Network> network_;
};
}  // namespace http
}  // namespace tests
}  // namespace olp