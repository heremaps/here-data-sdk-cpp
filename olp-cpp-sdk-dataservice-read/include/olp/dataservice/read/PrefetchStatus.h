/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <cstddef>

namespace olp {
namespace dataservice {
namespace read {

/// Represents the progress of a prefetch operation.
struct PrefetchStatus {
  /// Tiles available (prefetched).
  size_t prefetched_tiles;
  /// Total number of tiles to prefetch during prefetch operation.
  size_t total_tiles_to_prefetch;
  /// Total bytes tranferred during API calls.
  size_t bytes_transferred;
};

/// Represents the progress of a prefetch for partitions.
struct PrefetchPartitionsStatus {
  /// Partitions available (prefetched).
  size_t prefetched_partitions;
  /// Total number of partitions to prefetch during prefetch operation.
  size_t total_partitions_to_prefetch;
  /// Total bytes tranferred during API calls.
  size_t bytes_transferred;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
