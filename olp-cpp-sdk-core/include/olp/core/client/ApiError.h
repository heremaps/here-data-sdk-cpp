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

#include "ErrorCode.h"

#include <olp/core/CoreApi.h>
#include "olp/core/client/HttpResponse.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace client {
class CORE_API ApiError {
 public:
  /**
   * @brief ApiError Error information container
   */
  ApiError() = default;

  ApiError(const ErrorCode& error_code, const std::string& message,
           bool is_retryable = false)
      : error_code_(error_code),
        message_(message),
        is_retryable_(is_retryable) {
    if (error_code == ErrorCode::Cancelled) {
      http_status_code_ =
          static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR);
    }
  }

  ApiError(int http_status_code, const std::string& message = "")
      : error_code_(http::HttpStatusCode::GetErrorCode(http_status_code)),
        http_status_code_(http_status_code),
        message_(message),
        is_retryable_(http::HttpStatusCode::IsRetryable(http_status_code)) {}

  /**
   * Gets the error code
   */
  inline ErrorCode GetErrorCode() const { return error_code_; }

  /**
   * Gets the HTTP status code
   */
  inline int GetHttpStatusCode() const { return http_status_code_; }

  /**
   * Gets the error message.
   */
  inline const std::string& GetMessage() const { return message_; }

  /**
   * Returns true if a request can be retried for this error.
   */
  inline bool ShouldRetry() const { return is_retryable_; }

 private:
  // TODO: merge error_code_ and http_status_code_ by shifting ErrorCode values
  // to negatives
  ErrorCode error_code_{ErrorCode::Unknown};
  int http_status_code_{static_cast<int>(olp::http::ErrorCode::UNKNOWN_ERROR)};
  std::string message_;
  bool is_retryable_{false};
};
}  // namespace client
}  // namespace olp
