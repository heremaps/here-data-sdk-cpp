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

namespace olp {
namespace client {

/**
 * @brief Represents all possible errors that might happen during a user
 * request.
 */
enum class ErrorCode {
  /**
   * An unknown error. See the error message for details.
   */
  Unknown = 0,

  /**
   * The request was cancelled (usually by the user).
   */
  Cancelled,

  /**
   * The request passed invalid arguments.
   */
  InvalidArgument,

  /**
   * The request exceeded the timeout limit.
   */
  RequestTimeout,

  /**
   * An internal server failure.
   */
  InternalFailure,

  /**
   * The requested service is not available.
   */
  ServiceUnavailable,

  /**
   * The access to the service was denied due to insufficient credentials.
   */
  AccessDenied,

  /**
   * The URL malformed or some data parameters are not formed correctly.
   */
  BadRequest,

  /**
   * The conditions required to access the service are not met.
   */
  PreconditionFailed,

  /**
   * The requested resource was not found.
   */
  NotFound,

  /**
   * Too many requests sent in a given amount of time.
   */
  SlowDown,

  /**
   * The network connection error.
   */
  NetworkConnection
};

}  // namespace client
}  // namespace olp
