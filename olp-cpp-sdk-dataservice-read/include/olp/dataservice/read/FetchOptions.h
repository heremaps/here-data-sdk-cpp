/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
namespace dataservice {
namespace read {

enum FetchOptions {
  /**
   * A default option. Queries the network if the requested resource is not
   * found in the cache.
   */
  OnlineIfNotFound,

  /**
   * Skips cache lookups and queries the network right away.
   */
  OnlineOnly,

  /**
   * Returns immediately if a cache lookup fails.
   */
  CacheOnly,

  /**
   * Returns the requested cached resource if it is found and updates the cache
   * in the background.
   * @note Do not Use for versioned layer client requests.
   */
  CacheWithUpdate
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
