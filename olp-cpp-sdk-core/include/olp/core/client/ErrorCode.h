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
enum class ErrorCode {
  /**
   * Unknown error, see error message for details
   */
  Unknown = 0,

  /**
   * Request was cancelled (usually by the user)
   */
  Cancelled,

  /**
   * Request passed invalid aruguments
   */
  InvalidArgument,

  /**
   * Request exceeded timeout limit
   */
  RequestTimeout,

  /**
   * Internal server failure
   */
  InternalFailure,

  /**
   * Request service is unavailable
   */
  ServiceUnavailable,

  /**
   * Access denied to service due to insufficient credentials
   */
  AccessDenied,

  /**
   * Request malformed
   */
  BadRequest,

  /**
   * Conditions to access service are unmet
   */
  PreconditionFailed,

  /**
   * Requested resource not found
   */
  NotFound,

  /**
   * Too many requests sent in a given amount of time
   */
  SlowDown,

  /**
   * Network connection error detected
   */
  NetworkConnection
};
}
}  // namespace olp
