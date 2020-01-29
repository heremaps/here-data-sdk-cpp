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

#include <sstream>

#include <olp/core/CoreApi.h>
#include <olp/core/http/NetworkTypes.h>

namespace olp {
namespace client {

/**
 * @brief An HTTP response.
 */
class CORE_API HttpResponse {
 public:
  HttpResponse() = default;
  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   */
  HttpResponse(int status, std::string response = std::string())
      : status(status), response(std::move(response)) {}
  /**
   * @brief Creates the `HttpResponse` instance.
   *
   * @param status The HTTP status.
   * @param response The response body.
   */
  HttpResponse(int status, std::stringstream&& response)
      : status(status), response(std::move(response)) {}

  /**
   * @brief The HTTP status.
   *
   * @see `ErrorCode` for information on negative status
   * numbers.
   * @see `HttpStatusCode` for information on positive status
   * numbers.
   */
  int status{static_cast<int>(olp::http::ErrorCode::UNKNOWN_ERROR)};
  /**
   * @brief The HTTP response.
   */
  std::stringstream response;
};

}  // namespace client
}  // namespace olp
