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
namespace network2 {

/**
 * @brief Represents request id.
 * Values of this type are returned by Network::Send and used by
 * Network::Cancel.
 */
using RequestId = std::uint64_t;

/**
 * @brief List of special values for NetworkRequestId.
 */
enum class RequestIdConstants : std::uint64_t {
  /// Value that indicates invalid request id.
  RequestIdInvalid = std::numeric_limits<RequestId>::min(),
  /// Minimal value of valid request id.
  RequestIdMin = std::numeric_limits<RequestId>::min() + 1,
  /// Maximal value of valid request id.
  RequestIdMax = std::numeric_limits<RequestId>::max(),
};

}  // namespace network2
}  // namespace olp
