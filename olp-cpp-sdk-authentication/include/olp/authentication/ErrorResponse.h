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
 * @brief These stucts contain detailed descriptions of the errors returned as
 * part of the response for authentication operations.
 */
struct AUTHENTICATION_API ErrorResponse {
  /**
   * @brief Here Account error code returned as a result of a request. In case
   * of error code 400200 which specifies that invalid data was sent part of the
   * request, futher details can be retrieved by calling the error_fields method
   * of the response. See ErrorFields, ErrorField.
   */
  unsigned int code = 0ul;

  /**
   * @brief Here Account error message returned as a result of an authentication
   * request.
   */
  std::string message;
};

/**
 * @brief Input Field specific error
 */
struct AUTHENTICATION_API ErrorField : ErrorResponse {
  /**
   * @brief Name of the input field for which an invalid value was provided.
   */
  std::string name;
};

/**
 * @brief ErrorFields - List of ErrorField objects
 */
using ErrorFields = std::list<ErrorField>;

}  // namespace authentication
}  // namespace olp
