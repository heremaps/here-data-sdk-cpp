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

#include "olp/core/http/NetworkTypes.h"

namespace olp {
namespace http {

std::string ErrorCodeToString(ErrorCode code) {
  switch (code) {
    case ErrorCode::SUCCESS:
      return "Success";
    case ErrorCode::IO_ERROR:
      return "Input/Output error";
    case ErrorCode::AUTHORIZATION_ERROR:
      return "Authorization error";
    case ErrorCode::INVALID_URL_ERROR:
      return "Invalid URL";
    case ErrorCode::OFFLINE_ERROR:
      return "Offline";
    case ErrorCode::CANCELLED_ERROR:
      return "Cancelled";
    case ErrorCode::AUTHENTICATION_ERROR:
      return "Authentication error";
    case ErrorCode::TIMEOUT_ERROR:
      return "Timeout";
    case ErrorCode::NETWORK_OVERLOAD_ERROR:
      return "Network overload";
    default:
      return "Unknown error";
  }
}

}  // namespace http
}  // namespace olp