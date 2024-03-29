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
#include <limits>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {

/// Configures the behavior of the `StreamLayerClient` specific logic.
struct DATASERVICE_WRITE_API StreamLayerClientSettings {
  /**
   * @brief The maximum number of requests that can be stored.
   *
   * Make sure it is a positive number.
   */
  size_t maximum_requests = std::numeric_limits<size_t>::max();
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
