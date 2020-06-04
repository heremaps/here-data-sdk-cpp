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

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/VersionDependency.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief Represents a version info.
 */
class DATASERVICE_READ_API VersionInfo final {
 public:
  /**
   * @brief Sets the vector of version dependencies.
   *
   * @param vector of version dependencies.
   */
  void SetVersionDependencies(std::vector<VersionDependency> dependencies) {
    dependencies_ = std::move(dependencies);
  }

  /**
   * @brief Sets the map of partition counts.
   *
   * @param map of partition counts.
   */
  void SetPartitionCounts(
      std::map<std::string, std::int64_t> partition_counts) {
    partition_counts_ = std::move(partition_counts);
  }

  /**
   * @brief Sets the catalog version.
   *
   * @param catalog version.
   */
  void SetVersion(std::int64_t version) { version_ = version; }

  /**
   * @brief Sets the timestamp of catalog version.
   *
   * @param timestamp of catalog version.
   */
  void SetTimestamp(std::int64_t timestamp) { timestamp_ = timestamp; }

  /**
   * @brief Gets the vector of version dependencies.
   *
   * @return The vector of version dependencies.
   */
  const std::vector<VersionDependency>& GetVersionDependencies() const {
    return dependencies_;
  }

  /**
   * @brief Gets the map of partition counts.
   *
   * @return The map of partition counts.
   */
  const std::map<std::string, std::int64_t>& GetPartitionCounts() const {
    return partition_counts_;
  }

  /**
   * @brief Gets the catalog version.
   *
   * @return The catalog version.
   */
  std::int64_t GetVersion() const { return version_; }

  /**
   * @brief Gets the timestamp of catalog version.
   *
   * @return The timestamp of catalog version.
   */
  std::int64_t GetTimestamp() const { return timestamp_; }

 private:
  std::int64_t version_;
  std::int64_t timestamp_;
  std::vector<VersionDependency> dependencies_;
  std::map<std::string, std::int64_t> partition_counts_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
