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

namespace olp {
namespace authentication {
class Constants {
 public:
  // Error strings
  static const std::string ERROR_HTTP_NO_CONTENT;
  static const std::string ERROR_HTTP_NOT_FOUND;
  static const std::string ERROR_HTTP_SERVICE_UNAVAILABLE;

  static const unsigned int INVALID_DATA_ERROR_CODE;

  // JSON strings
  static const char* ACCESS_TOKEN;
  static const char* EXPIRES_IN;
  static const char* REFRESH_TOKEN;
};

}  // namespace authentication
}  // namespace olp
