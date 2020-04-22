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

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <olp/core/CoreApi.h>

namespace olp {
namespace http {

/**
 * @brief A unique request ID.
 *
 * Values of this type mark a unique request all the way until the request
 * completion. This value is returned by `Network::Send` and used by
 * `Network::Cancel` and `NetworkResponse` so that the user can track
 * the request until its completion.
 */
using RequestId = std::uint64_t;

/**
 * @brief The list of special values for `NetworkRequestId`.
 */
enum class RequestIdConstants : RequestId {
  /// The value that indicates the invalid request ID.
  RequestIdInvalid = std::numeric_limits<RequestId>::min(),
  /// The minimum value of the valid request ID.
  RequestIdMin = std::numeric_limits<RequestId>::min() + 1,
  /// The maximum value of the valid request ID.
  RequestIdMax = std::numeric_limits<RequestId>::max(),
};

/**
 * @brief The common `Network` error codes.
 */
enum class ErrorCode {
  SUCCESS = 0,
  IO_ERROR = -1,
  AUTHORIZATION_ERROR = -2,
  INVALID_URL_ERROR = -3,
  OFFLINE_ERROR = -4,
  CANCELLED_ERROR = -5,
  AUTHENTICATION_ERROR = -6,
  TIMEOUT_ERROR = -7,          /*!< The timeout interval of the request expired
                                before request was completed. */
  NETWORK_OVERLOAD_ERROR = -8, /*!< Reached maximum limit of active requests
                                that network can process. */
  UNKNOWN_ERROR = -9,          ///< Internal error that can't be interpreted.
};

/**
 * @brief Rrepresents the outcome of a network request.
 *
 * It contains either a valid request ID or error code if the request
 * trigger failed. The caller must check whether the outcome of the request was
 * a success before attempting to access the result or error.
 */
class CORE_API SendOutcome final {
 public:
  /// The invalid request ID alias.
  static constexpr RequestId kInvalidRequestId =
      static_cast<RequestId>(RequestIdConstants::RequestIdInvalid);

  /**
   * @brief Sets a successful request outcome.
   *
   * @param request_id The valid unique request ID.
   */
  explicit SendOutcome(RequestId request_id) : request_id_(request_id) {}

  /**
   * @brief Sets an unsuccessful request outcome.
   *
   * @param error_code The error code that specifies why the request failed.
   */
  explicit SendOutcome(ErrorCode error_code) : error_code_(error_code) {}

  /**
   * @brief Checks if the network request push was successful.
   *
   * @return True if there is no error and the request ID is valid; false
   * otherwise.
   */
  bool IsSuccessful() const {
    return error_code_ == ErrorCode::SUCCESS &&
           request_id_ != kInvalidRequestId;
  }

  /**
   * @brief Gets the request ID.
   *
   * @return The valid request ID if the request was successful;
   * `RequestIdConstants::RequestIdInvalid` otherwise.
   */
  RequestId GetRequestId() const { return request_id_; }

  /**
   * @brief Gets the error code.
   *
   * @return `ErrorCode::SUCCESS` if the request was successful; any other
   * `ErrorCode` otherwise.
   */
  ErrorCode GetErrorCode() const { return error_code_; }

 private:
  /// The request ID.
  RequestId request_id_{kInvalidRequestId};
  /// The error code.
  ErrorCode error_code_{ErrorCode::SUCCESS};
};

/**
 * @brief The helper function that converts an error code to a human readable
 * string.
 */
CORE_API std::string ErrorCodeToString(ErrorCode code);

/**
 * @brief The type alias for the HTTP header.
 */
using Header = std::pair<std::string, std::string>;

/**
 * @brief The type alias for a vector of HTTP headers.
 */
using Headers = std::vector<Header>;

}  // namespace http
}  // namespace olp
