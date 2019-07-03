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

#include "PartitionTimestampQueue.h"

namespace olp {
namespace dataservice {
namespace write {
std::chrono::milliseconds CalculateTimeSinceOldestPartition(
    const PartitionTimestampQueue& queue) {
  auto top_timestamp = queue.top(size_t{});
  if (top_timestamp == std::chrono::system_clock::time_point{}) {
    return {};
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - top_timestamp);
}

void PushPartitionTimestamps(PartitionTimestampQueue& queue, size_t size) {
  queue.emplace_count(size, std::chrono::system_clock::now());
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
