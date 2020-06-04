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

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief Represents a catalog version dependencies
 */
class DATASERVICE_READ_API CatalogVersionDependencies final {
 public:
  /**
   * @brief Sets the hrn.
   *
   * @param hrn.
   */
  void SetHrn(std::string hrn) { hrn_ = std::move(hrn); }

  /**
   * @brief Sets the version.
   *
   * @param version.
   */
  void SetVersion(int64_t version) { version_ = version; }

  /**
   * @brief Sets the direct status.
   *
   * @param direct status.
   */
  void SetDirect(bool direct) { direct_ = direct; }

  /**
   * @brief Gets the hrn.
   *
   * @return The hrn.
   */
  const std::string& GetHrn() const { return hrn_; }

  /**
   * @brief Gets the version.
   *
   * @return The version.
   */
  int64_t GetVersion() const { return version_; }

  /**
   * @brief Gets the direct status.
   *
   * @return The direct status.
   */
  bool IsDirect() const { return direct_; }

 private:
  std::string hrn_;
  int64_t version_;
  bool direct_;
};

/**
 * @brief Represents a catalog version
 */
class DATASERVICE_READ_API CatalogVersion final {
 public:
  /**
   * @brief Sets the vector of catalog version dependencies.
   *
   * @param vector of catalog version dependencies.
   */
  void SetCatalogVersionDependencies(
      std::vector<CatalogVersionDependencies> dependencies) {
    dependencies_ = std::move(dependencies);
  }

  /**
   * @brief Sets the map of partition counts.
   *
   * @param map of partition counts.
   */
  void SetPartitionCounts(std::map<std::string, int64_t> partition_counts) {
    partition_counts_ = std::move(partition_counts);
  }

  /**
   * @brief Sets the catalog version.
   *
   * @param catalog version.
   */
  void SetVersion(int64_t version) { version_ = version; }

  /**
   * @brief Sets the timestamp of catalog version.
   *
   * @param timestamp of catalog version.
   */
  void SetTimestamp(int64_t timestamp) { timestamp_ = timestamp; }

  /**
   * @brief Gets the vector of catalog version dependencies.
   *
   * @return The vector of catalog version dependencies.
   */
  const std::vector<CatalogVersionDependencies>& GetCatalogVersionDependencies()
      const {
    return dependencies_;
  }

  /**
   * @brief Gets the map of partition counts.
   *
   * @return The map of partition counts.
   */
  const std::map<std::string, int64_t>& GetPartitionCounts() const {
    return partition_counts_;
  }

  /**
   * @brief Gets the catalog version.
   *
   * @return The catalog version.
   */
  int64_t GetVersion() const { return version_; }

  /**
   * @brief Gets the timestamp of catalog version.
   *
   * @return The timestamp of catalog version.
   */
  int64_t GetTimestamp() const { return timestamp_; }

 private:
  int64_t version_;
  int64_t timestamp_;
  std::vector<CatalogVersionDependencies> dependencies_;
  std::map<std::string, int64_t> partition_counts_;
};
/**
 * @brief A container for list of versions.
 */
class DATASERVICE_READ_API Versions final {
 public:
  /**
   * @brief Sets the vector of versions.
   *
   * @param value The vector of catalog versions .
   */
  void SetCatalogVersion(std::vector<CatalogVersion> value) {
    versions_ = std::move(value);
  }

  /**
   * @brief Gets the vector of catalog versions.
   *
   * @return The vector of catalog versions.
   */
  const std::vector<CatalogVersion>& GetCatalogVersions() const {
    return versions_;
  }

 private:
  std::vector<CatalogVersion> versions_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
