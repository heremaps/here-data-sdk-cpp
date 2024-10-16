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

#include "olp/core/client/RetrySettings.h"

#include <random>

#include "olp/core/client/HttpResponse.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace client {

bool DefaultRetryCondition(const HttpResponse& response) {
  const auto status = response.GetStatus();
  if ((status >= http::HttpStatusCode::INTERNAL_SERVER_ERROR &&
       status <= http::HttpStatusCode::NETWORK_CONNECT_TIMEOUT) ||
      status == http::HttpStatusCode::TOO_MANY_REQUESTS) {
    return true;
  }

  if (status == static_cast<int>(http::ErrorCode::IO_ERROR) ||
      status == static_cast<int>(http::ErrorCode::OFFLINE_ERROR) ||
      status == static_cast<int>(http::ErrorCode::TIMEOUT_ERROR) ||
      status == static_cast<int>(http::ErrorCode::NETWORK_OVERLOAD_ERROR)) {
    return true;
  }
  return false;
}

}  // namespace client
}  // namespace olp
