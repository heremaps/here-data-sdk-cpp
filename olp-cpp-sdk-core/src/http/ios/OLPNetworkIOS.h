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

#include <mutex>

#include "olp/core/http/Network.h"

#ifdef __OBJC__
@class OLPHttpClient;
using OLPHttpClientPtr = OLPHttpClient*;
#else
using OLPHttpClientPtr = void*;
#endif

namespace olp {
namespace http {

/**
 * @brief The implementation of http interface on iOS using NSURLSession
 */
class OLPNetworkIOS : public olp::http::Network {
 public:
  explicit OLPNetworkIOS(size_t max_requests_count);
  ~OLPNetworkIOS();

  olp::http::SendOutcome Send(
      olp::http::NetworkRequest request,
      std::shared_ptr<std::ostream> payload,
      olp::http::Network::Callback callback,
      olp::http::Network::HeaderCallback header_callback = nullptr,
      olp::http::Network::DataCallback data_callback = nullptr) override;

  void Cancel(olp::http::RequestId identifier) override;

 private:
  void Cleanup();

  olp::http::RequestId GenerateNextRequestId();

 private:
  const size_t max_requests_count_;
  OLPHttpClientPtr http_client_{nullptr};
  std::mutex mutex_;
  RequestId request_id_counter_{
      static_cast<RequestId>(RequestIdConstants::RequestIdMin)};
};
}  // namespace http
}  // namespace olp
