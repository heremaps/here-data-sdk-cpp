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

#include <list>
#include <string>

#include "AuthenticationApi.h"

namespace olp {
namespace authentication {
/**
 * @brief Detailed descriptions of errors returned as
 * a response to an authentication operation.
 *
 * You can get the following information on the errors: the name of the field
 * with the invalid value, error code, message, and ID.
 */
struct AUTHENTICATION_API ErrorResponse {
  /**
   * @brief The HERE Account error code returned as a result of
   * the authentication request. In case of the error code 400200 that specifies
   * that invalid data was sent as a part of the request, further details can be
   * retrieved by calling the `error_fields` method of the response.
   * @see `ErrorFields`, `ErrorField`.
   */
  unsigned int code = 0ul;

  /**
   * @brief The HERE Account error message returned as a result of
   * the authentication request.
   */
  std::string message;

  /**
   * @brief The HERE Account error ID returned as a result of the authentication
   * request.
   */
  std::string error_id;
};

/**
 * @brief An input field error.
 */
struct AUTHENTICATION_API ErrorField : ErrorResponse {
  /**
   * @brief The name of the input field with the invalid value.
   */
  std::string name;
};

/**
 * @brief The list of the `ErrorField` objects.
 */
using ErrorFields = std::list<ErrorField>;

}  // namespace authentication
}  // namespace olp
