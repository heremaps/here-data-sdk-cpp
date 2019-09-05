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

#include "olp/core/client/ErrorCode.h"
#include "olp/core/http/NetworkTypes.h"

namespace olp {
namespace http {

/**
 * @brief HTTP status codes as specified by RFC7231.
 * For more information see
 * https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml
 */
class HttpStatusCode {
 public:
  enum : int {
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING = 102,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTI_STATUS = 207,
    ALREADY_REPORTED = 208,
    IM_USED = 226,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    SWITCH_PROXY = 306,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    REQUEST_ENTITY_TOO_LARGE = 413,
    REQUEST_URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    IM_A_TEAPOT = 418,
    AUTHENTICATION_TIMEOUT = 419,
    METHOD_FAILURE = 420,
    UNPROC_ENTITY = 422,
    LOCKED = 423,
    FAILED_DEPENDENCY = 424,
    UPGRADE_REQUIRED = 426,
    PRECONDITION_REQUIRED = 427,
    TOO_MANY_REQUESTS = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    LOGIN_TIMEOUT = 440,
    NO_RESPONSE = 444,
    RETRY_WITH = 449,
    BLOCKED = 450,
    REDIRECT = 451,
    REQUEST_HEADER_TOO_LARGE = 494,
    CERTIFICATE = 495,
    NO_CERTIFICATE = 496,
    HTTP_TO_HTTPS_PORT = 497,
    CLIENT_CLOSED_REQUEST = 499,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    VERSION_NOT_SUPPORTED = 505,
    VARIANT_ALSO_NEGOTIATES = 506,
    INSUFFICIENT_STORAGE = 506,
    LOOP_DETECTED = 508,
    BANDWIDTH_LIMIT_EXCEEDED = 509,
    NOT_EXTENDED = 510,
    NETWORK_AUTHENTICATION_REQUIRED = 511,
    NETWORK_READ_TIMEOUT = 598,
    NETWORK_CONNECT_TIMEOUT = 599,
  };

  /**
   * @brief Check if the given error code is temporary error.
   * Some errors are sever or due to wrong user data an thus cannot be repeated,
   * respectively will always result in the same error.
   * @param[in] http_code A HTTP status code or a \c olp::http::ErrorCode.
   * @return \c true in case request can be repeated, \c false otherwise.
   */
  static bool IsRetryable(int http_code) {
    if (http_code < 0) return false;

    switch (http_code) {
      case HttpStatusCode::INTERNAL_SERVER_ERROR:
      case HttpStatusCode::SERVICE_UNAVAILABLE:
      case HttpStatusCode::TOO_MANY_REQUESTS:
      case HttpStatusCode::BANDWIDTH_LIMIT_EXCEEDED:
      case HttpStatusCode::REQUEST_TIMEOUT:
      case HttpStatusCode::AUTHENTICATION_TIMEOUT:
      case HttpStatusCode::LOGIN_TIMEOUT:
      case HttpStatusCode::GATEWAY_TIMEOUT:
      case HttpStatusCode::NETWORK_READ_TIMEOUT:
      case HttpStatusCode::NETWORK_CONNECT_TIMEOUT:
        return true;
      default:
        return false;
    }
  }

  /**
   * @brief Maps \c olp::http::ErrorCode or HTTP status code to a
   * \c olp::client::ErrorCode.
   * @param[in] http_code A HTTP status code or a \c olp::http::ErrorCode.
   * @return The coresponding \c olp::client::ErrorCode.
   */
  static olp::client::ErrorCode GetErrorCode(int http_code) {
    // ErrorCode (negative numbers)
    if (http_code < 0) {
      switch (static_cast<olp::http::ErrorCode>(http_code)) {
        case olp::http::ErrorCode::OFFLINE_ERROR:
        case olp::http::ErrorCode::IO_ERROR:
          return olp::client::ErrorCode::NetworkConnection;
        case olp::http::ErrorCode::TIMEOUT_ERROR:
          return olp::client::ErrorCode::RequestTimeout;
        case olp::http::ErrorCode::CANCELLED_ERROR:
          return olp::client::ErrorCode::Cancelled;
        case olp::http::ErrorCode::AUTHORIZATION_ERROR:
        case olp::http::ErrorCode::AUTHENTICATION_ERROR:
          return olp::client::ErrorCode::AccessDenied;
        case olp::http::ErrorCode::INVALID_URL_ERROR:
          return olp::client::ErrorCode::ServiceUnavailable;
        case olp::http::ErrorCode::UNKNOWN_ERROR:
        default:
          return olp::client::ErrorCode::Unknown;
      }
    }

    // HttpStatusCode
    switch (http_code) {
      case HttpStatusCode::BAD_REQUEST:
        return olp::client::ErrorCode::BadRequest;
      case HttpStatusCode::UNAUTHORIZED:
      case HttpStatusCode::FORBIDDEN:
        return olp::client::ErrorCode::AccessDenied;
      case HttpStatusCode::NOT_FOUND:
        return olp::client::ErrorCode::NotFound;
      case HttpStatusCode::PRECONDITION_FAILED:
        return olp::client::ErrorCode::PreconditionFailed;
      case HttpStatusCode::TOO_MANY_REQUESTS:
      case HttpStatusCode::BANDWIDTH_LIMIT_EXCEEDED:
        return olp::client::ErrorCode::SlowDown;
      case HttpStatusCode::INTERNAL_SERVER_ERROR:
        return olp::client::ErrorCode::InternalFailure;
      case HttpStatusCode::SERVICE_UNAVAILABLE:
        return olp::client::ErrorCode::ServiceUnavailable;
      case HttpStatusCode::REQUEST_TIMEOUT:
      case HttpStatusCode::AUTHENTICATION_TIMEOUT:
      case HttpStatusCode::LOGIN_TIMEOUT:
      case HttpStatusCode::GATEWAY_TIMEOUT:
      case HttpStatusCode::NETWORK_READ_TIMEOUT:
      case HttpStatusCode::NETWORK_CONNECT_TIMEOUT:
        return olp::client::ErrorCode::RequestTimeout;
      default:
        return olp::client::ErrorCode::Unknown;
    }
  }
};

}  // namespace http
}  // namespace olp
