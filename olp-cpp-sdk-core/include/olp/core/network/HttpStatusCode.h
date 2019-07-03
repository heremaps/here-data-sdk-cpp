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
#include "olp/core/network/Network.h"

namespace olp {
namespace network {

/**
 * @brief HTTP status codes
 */
class HttpStatusCode {
 public:
  enum : int {
    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    ImUsed = 226,
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    SwitchProxy = 306,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    RequestEntityTooLarge = 413,
    RequestUriTooLong = 414,
    UnsupportedMediaType = 415,
    RequestedRangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    AuthenticationTimeout = 419,
    MethodFailure = 420,
    UnprocEntity = 422,
    Locked = 423,
    FailedDependency = 424,
    UpgradeRequired = 426,
    PreconditionRequired = 427,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    LoginTimeout = 440,
    NoResponse = 444,
    RetryWith = 449,
    Blocked = 450,
    Redirect = 451,
    RequestHeaderTooLarge = 494,
    Certificate = 495,
    NoCertificate = 496,
    HttpToHttpsPort = 497,
    ClientClOsedToRequest = 499,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    VersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 506,
    LoopDetected = 508,
    BandwidthLimitExceeded = 509,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511,
    NetworkReadTimeout = 598,
    NetworkConnectTimeout = 599
  };

  static bool IsRetryableHttpStatusCode(int http_code) {
    if (http_code < 0) return false;

    switch (http_code) {
      case HttpStatusCode::InternalServerError:
      case HttpStatusCode::ServiceUnavailable:
      case HttpStatusCode::TooManyRequests:
      case HttpStatusCode::BandwidthLimitExceeded:
      case HttpStatusCode::RequestTimeout:
      case HttpStatusCode::AuthenticationTimeout:
      case HttpStatusCode::LoginTimeout:
      case HttpStatusCode::GatewayTimeout:
      case HttpStatusCode::NetworkReadTimeout:
      case HttpStatusCode::NetworkConnectTimeout:
        return true;
      default:
        return false;
    }
  }

  // Best effort attempt to map HTTP response codes to ErrorCode
  static olp::client::ErrorCode GetErrorForHttpStatusCode(int http_code) {
    if (http_code < 0) {
      switch (http_code) {
        case Network::ErrorCode::Offline:
        case Network::ErrorCode::IOError:
          return olp::client::ErrorCode::NetworkConnection;
        case Network::ErrorCode::TimedOut:
          return olp::client::ErrorCode::RequestTimeout;

        case Network::ErrorCode::Cancelled:
          return olp::client::ErrorCode::Cancelled;

        case Network::ErrorCode::AuthorizationError:
        case Network::ErrorCode::AuthenticationError:
          return olp::client::ErrorCode::AccessDenied;

        case Network::ErrorCode::InvalidURLError:
          return olp::client::ErrorCode::ServiceUnavailable;

        case Network::ErrorCode::UnknownError:
        default:
          return olp::client::ErrorCode::Unknown;
      }
    }

    switch (http_code) {
      case HttpStatusCode::BadRequest:
        return olp::client::ErrorCode::BadRequest;
      case HttpStatusCode::Unauthorized:
      case HttpStatusCode::Forbidden:
        return olp::client::ErrorCode::AccessDenied;
      case HttpStatusCode::NotFound:
        return olp::client::ErrorCode::NotFound;
      case HttpStatusCode::PreconditionFailed:
        return olp::client::ErrorCode::PreconditionFailed;
      case HttpStatusCode::TooManyRequests:
      case HttpStatusCode::BandwidthLimitExceeded:
        return olp::client::ErrorCode::SlowDown;
      case HttpStatusCode::InternalServerError:
        return olp::client::ErrorCode::InternalFailure;
      case HttpStatusCode::ServiceUnavailable:
        return olp::client::ErrorCode::ServiceUnavailable;
      case HttpStatusCode::RequestTimeout:
      case HttpStatusCode::AuthenticationTimeout:
      case HttpStatusCode::LoginTimeout:
      case HttpStatusCode::GatewayTimeout:
      case HttpStatusCode::NetworkReadTimeout:
      case HttpStatusCode::NetworkConnectTimeout:
        return olp::client::ErrorCode::RequestTimeout;
      default:
        return olp::client::ErrorCode::Unknown;
    }
  }
};

}  // namespace network
}  // namespace olp
