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

#include <string>

#include "AuthenticationApi.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace authentication {
class AUTHENTICATION_API AuthenticationError {
 public:
  /**
   * @brief Authentication error information container
   */
  AuthenticationError() = default;

  AuthenticationError(int network_code, const std::string& message)
      : error_code_(http::HttpStatusCode::GetErrorCode(network_code))
      , message_(message)
      , is_retryable_(IsRetryableNetworkCode(network_code)) {}

  /**
   * Gets the error code
   */
  inline const client::ErrorCode& GetErrorCode() const { return error_code_; }

  /**
   * Gets the error message.
   */
  inline const std::string& GetMessage() const { return message_; }

  /**
   * Returns true if a request can be retried for this error.
   */
  inline bool ShouldRetry() const { return is_retryable_; }

 private:
  static bool IsRetryableNetworkCode(int network_code) {
    switch (static_cast<olp::http::ErrorCode>(network_code)) {
      case olp::http::ErrorCode::OFFLINE_ERROR:
      case olp::http::ErrorCode::CANCELLED_ERROR:
      case olp::http::ErrorCode::TIMEOUT_ERROR:
        return true;
      default:
        return false;
    }
  }

 private:
  client::ErrorCode error_code_{client::ErrorCode::Unknown};
  std::string message_{};
  bool is_retryable_{false};
};

}  // namespace authentication
}  // namespace olp
