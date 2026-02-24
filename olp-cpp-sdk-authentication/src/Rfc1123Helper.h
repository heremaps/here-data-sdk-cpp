/*
 * Copyright (C) 2026 HERE Europe B.V.
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

#include <ctime>
#include <string>

namespace olp {
namespace authentication {
namespace internal {

// Parses RFC1123 date string and returns time_t. In case of parsing error,
// returns -1. Handrolled parser that: 1) works on all platforms (std::get_time
// is not supported in some environments, e.g. older libstdc++) 2) does not
// throw exceptions (boost::date_time can throw on parsing errors) 3) ignores
// locale
std::time_t ParseRfc1123GmtNoExceptions(const std::string& value);

}  // namespace internal
}  // namespace authentication
}  // namespace olp
