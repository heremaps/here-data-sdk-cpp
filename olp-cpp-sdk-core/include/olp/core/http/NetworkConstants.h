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

namespace olp {
namespace http {

/**
 * HTTP Headers
 */
static constexpr auto kAuthorizationHeader = "Authorization";
static constexpr auto kContentTypeHeader = "Content-Type";
static constexpr auto kUserAgentHeader = "User-Agent";

/**
 * Custom constants
 */
static constexpr auto kBearer = "Bearer";
static constexpr auto kOlpSdkUserAgent =
    "OLP-CPP-SDK/" EDGE_SDK_VERSION_STRING " (" EDGE_SDK_PLATFORM_NAME ")";

}  // namespace http
}  // namespace olp
