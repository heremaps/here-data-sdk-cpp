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

#include <olp/core/client/ApiError.h>
#include "AuthenticationApi.h"

namespace olp {
namespace authentication {

/**
 * @brief Contains information on the authentication error.
 *
 * You can get the following information on the authentication error: the error
 * code ( \ref GetErrorCode ) and error message ( \ref GetMessage ).
 *
 * @deprecated Will be removed by 12.2020. Use `client::ApiError` instead.
 */
using AuthenticationError = client::ApiError;

}  // namespace authentication
}  // namespace olp
