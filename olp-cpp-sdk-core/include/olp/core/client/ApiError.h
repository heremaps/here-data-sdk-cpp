/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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
#include <utility>

#include "ErrorCode.h"

#include <olp/core/CoreApi.h>
#include "olp/core/client/HttpResponse.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace client {

/**
 * @brief A wrapper around an internal error or HTTP status code.
 */
class CORE_API ApiError {
 public:
  /**
   * @brief Creates the `ApiError` instance with the cancelled error code and
   * description.
   *
   * @param message The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError Cancelled(const char* message = "Cancelled") {
    return {ErrorCode::Cancelled, message};
  }

  /**
   * @brief Creates the `ApiError` instance with the network connection error
   * code and description.
   *
   * @param message The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError NetworkConnection(const char* message = "Offline") {
    return {ErrorCode::NetworkConnection, message};
  }

  /**
   * @brief Creates the `ApiError` instance with the precondition failed error
   * code and description.
   *
   * @param message The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError PreconditionFailed(
      const char* message = "Precondition failed") {
    return {ErrorCode::PreconditionFailed, message};
  }

  /**
   * @brief Creates the `ApiError` instance with the invalid argument error code
   * and description.
   *
   * @param message The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError InvalidArgument(const char* message = "Invalid argument") {
    return {ErrorCode::InvalidArgument, message};
  }

  /**
   * @brief Creates the `ApiError` instance with the not found error code and
   * description.
   *
   * @param description The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError NotFound(const char* message = "Resource not found") {
    return {ErrorCode::NotFound, message};
  }

  /**
   * @brief Creates the `ApiError` instance with the cache IO error code and
   * description.
   *
   * @param description The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError CacheIO(const char* description = "Cache IO") {
    return {ErrorCode::CacheIO, description};
  }

  /**
   * @brief Creates the `ApiError` instance with the offline error code and
   * description.
   *
   * @param description The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError Offline(const char* description = "Offline") {
    return {ErrorCode::Offline, description};
  }

  /**
   * @brief Creates the `ApiError` instance with the unknown error code and
   * description.
   *
   * @param description The optional description.
   *
   * @return The `ApiError` instance.
   */
  static ApiError Unknown(const char* message = "Unknown") {
    return {ErrorCode::Unknown, message};
  }

  ApiError() = default;

  /**
   * @brief Creates the `ApiError` instance with the internal error.
   *
   * Represents the internal error that is not related to any HTTP status
   * returned during the request. You can call this constructor using the error
   * code and error message.
   *
   * @param error_code The internal error code.
   * @param message The text message of the error.
   * @param is_retryable Indicates if the error is permanent or temporary
   * and if the user can retry the operation.
   */
  ApiError(ErrorCode error_code, std::string message, bool is_retryable = false)
      : error_code_(error_code),
        message_(std::move(message)),
        is_retryable_(is_retryable) {
    if (error_code == ErrorCode::Cancelled) {
      http_status_code_ =
          static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR);
    }
  }

  /**
   * @brief Creates the `ApiError` instance with the HTTP status code.
   *
   * Represents the server status. Evaluates the HTTP status code and sets
   * the `error_code_` and `is_retriable_ flag` parameters. You can call this
   * constructor using the HTTP status code and error text message.
   *
   * @param http_status_code The HTTP status code returned by the server.
   * @param message The text message of the error.
   */
  ApiError(int http_status_code, std::string message = "")  // NOLINT
      : error_code_(http::HttpStatusCode::GetErrorCode(http_status_code)),
        http_status_code_(http_status_code),
        message_(std::move(message)),
        is_retryable_(http::HttpStatusCode::IsRetryable(http_status_code)) {}

  /**
   * @brief Gets the error code.
   *
   * @return The code associated with the error.
   */
  inline ErrorCode GetErrorCode() const { return error_code_; }

  /**
   * @brief Gets the HTTP status code.
   *
   * @return The HTTP status code.
   */
  inline int GetHttpStatusCode() const { return http_status_code_; }

  /**
   * @brief Gets the error message.
   *
   * @return The error message associated with the error.
   */
  inline const std::string& GetMessage() const { return message_; }

  /**
   * @brief Checks if the request can be retried for this error.
   *
   * @return True if the request can be retried for this error; false otherwise.
   */
  inline bool ShouldRetry() const { return is_retryable_; }

 private:
  // TODO: merge error_code_ and http_status_code_ by shifting ErrorCode values
  // to negatives
  ErrorCode error_code_{ErrorCode::Unknown};
  int http_status_code_{static_cast<int>(http::ErrorCode::UNKNOWN_ERROR)};
  std::string message_;
  bool is_retryable_{false};
};

}  // namespace client
}  // namespace olp
