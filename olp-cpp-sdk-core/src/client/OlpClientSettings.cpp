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

#include "olp/core/client/OlpClientSettings.h"

#include <random>

#include "olp/core/client/HttpResponse.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace client {

bool DefaultRetryCondition(const HttpResponse& response) {
  if ((response.status >= http::HttpStatusCode::INTERNAL_SERVER_ERROR &&
       response.status <= http::HttpStatusCode::NETWORK_CONNECT_TIMEOUT) ||
      response.status == http::HttpStatusCode::TOO_MANY_REQUESTS) {
    return true;
  }
  return false;
}

}  // namespace client
}  // namespace olp
