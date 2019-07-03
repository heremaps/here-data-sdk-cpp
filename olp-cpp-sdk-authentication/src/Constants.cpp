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

#include "Constants.h"

namespace olp {
namespace authentication {
const std::string Constants::ERROR_HTTP_NO_CONTENT = "No content";
const std::string Constants::ERROR_HTTP_NOT_FOUND =
    "Requested entity not found";
const std::string Constants::ERROR_HTTP_SERVICE_UNAVAILABLE =
    "Service unavailable";

const unsigned int Constants::INVALID_DATA_ERROR_CODE = 400200;

const char* Constants::ACCESS_TOKEN = "accessToken";
const char* Constants::EXPIRES_IN = "expiresIn";
const char* Constants::REFRESH_TOKEN = "refreshToken";
}  // namespace authentication
}  // namespace olp
