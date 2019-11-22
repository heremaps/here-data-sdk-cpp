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
  Http2HttpNetworkWrapper()
      : network_{std::move(olp::http::CreateDefaultNetwork(32))} {}

  SendOutcome Send(NetworkRequest request, Payload payload, Callback callback,
                   HeaderCallback header_callback = nullptr,
                   DataCallback data_callback = nullptr) override {
    ReplaceHttps2Http(request);
    InsertDebugHeaders(request);

    return network_->Send(std::move(request), std::move(payload),
                          std::move(callback), std::move(header_callback),
                          std::move(data_callback));
  }

  void Cancel(RequestId id) override { network_->Cancel(id); }

  /*
   * Adds special header, which signal mock server to generate timeouts and
   * stalls when serving requests.
   */
  void EnableTimeouts(bool with_timeouts) { with_timeouts_ = with_timeouts; }

  /*
   * Adds special header, which signal mock server to generate errors when
   * serving requests. Error rate is 10%.
   */
  void EnableErrors(bool with_errors) { with_errors_ = with_errors; }

 private:
  static void ReplaceHttps2Http(NetworkRequest &request) {
    auto url = request.GetUrl();
    auto pos = url.find("https");
    if (pos != std::string::npos) {
      url.replace(pos, 5, "http");
      request.WithUrl(url);
    }
  }

  /*
   * Note: headers with empty values are optimized out.
   */
  void InsertDebugHeaders(NetworkRequest &request) {
    if (with_errors_) {
      request.WithHeader("debug-with-errors", "Ok");
    }

    if (with_timeouts_) {
      request.WithHeader("debug-with-timeouts", "Ok");
    }
  }

  bool with_timeouts_ = false;
  bool with_errors_ = false;
  std::shared_ptr<Network> network_;
};
}  // namespace http
}  // namespace tests
}  // namespace olp