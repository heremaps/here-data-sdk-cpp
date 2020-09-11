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

#include <string>
#include <vector>

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief Represents the result of a prefetch operation for partitions.
 */
class DATASERVICE_READ_API PrefetchedPartitions {
 public:
  /**
   * @brief A parent type of the current `PrefetchedPartitions` class.
   */
  PrefetchedPartitions() = default;

  /**
   * @brief Adds the partition ID that was successfully downloaded.
   *
   * @param partition The partition ID.
   */
  void AddPartition(std::string&& partition) {
    partitions.emplace_back(partition);
  }

  /**
   * @brief Adds the partition ID that was successfully downloaded.
   *
   * @param partition The partition ID.
   */
  void AddPartition(const std::string& partition) {
    partitions.push_back(partition);
  }

  /**
   * @brief Gets the list of prefetched partitions.
   *
   * @return The list of partitions.
   */
  const std::vector<std::string>& GetPartitions() const { return partitions; }

  /**
   * @brief Moves the list of prefetched partitions.
   *
   * @return A forwarding reference to the partitions.
   */
  std::vector<std::string>&& MovePartitions() { return std::move(partitions); }

 private:
  std::vector<std::string> partitions;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
