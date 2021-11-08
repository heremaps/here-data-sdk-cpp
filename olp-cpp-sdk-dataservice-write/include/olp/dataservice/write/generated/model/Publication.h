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
#include <vector>

#include <boost/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/Details.h>
#include <olp/dataservice/write/generated/model/VersionDependency.h>


namespace olp {
namespace dataservice {
namespace write {
namespace model {

/// Contains information on a publication.
class DATASERVICE_WRITE_API Publication {
 public:
  Publication() = default;
  virtual ~Publication() = default;

 private:
  boost::optional<std::string> id_;
  boost::optional<Details> details_;
  boost::optional<std::vector<std::string>> layer_ids_;
  boost::optional<int64_t> catalog_version_;
  boost::optional<std::vector<VersionDependency>> version_dependencies_;

 public:
  /**
   * @brief Gets the publication ID.
   *
   * @return The publication ID.
   */
  const boost::optional<std::string>& GetId() const { return id_; }

  /**
   * @brief Gets a mutable reference to the publication ID.
   *
   * @return The mutable reference to the publication ID.
   */
  boost::optional<std::string>& GetMutableId() { return id_; }

  /**
   * @brief Sets the publication ID.
   *
   * @param value The publication ID.
   */
  void SetId(const std::string& value) { this->id_ = value; }

  /**
   * @brief Gets the details of the publication.
   *
   * @return The details of the publication.
   */
  const boost::optional<Details>& GetDetails() const { return details_; }

  /**
   * @brief Gets a mutable reference to the details of the publication.
   *
   * @return The mutable reference to the details of the publication.
   */
  boost::optional<Details>& GetMutableDetails() { return details_; }

  /**
   * @brief Sets the details of the publication.
   *
   * @param value The details of the publication.
   */
  void SetDetails(const Details& value) { this->details_ = value; }

  /**
   * @brief Gets the ID of the layer that should be published.
   *
   * @return The layer ID.
   */
  const boost::optional<std::vector<std::string>>& GetLayerIds() const {
    return layer_ids_;
  }

  /**
   * @brief Gets a mutable reference to the ID of the layer that should be published.
   *
   * @return The mutable reference to the layer ID.
   */
  boost::optional<std::vector<std::string>>& GetMutableLayerIds() {
    return layer_ids_;
  }

  /**
   * @brief Sets the layer ID.
   *
   * @param value The layer ID.
   */
  void SetLayerIds(const std::vector<std::string>& value) {
    this->layer_ids_ = value;
  }

  /**
   * @brief Gets the version of the catalog to be published.
   *
   * @return The catalog version.
   */
  const boost::optional<int64_t>& GetCatalogVersion() const {
    return catalog_version_;
  }

  /**
   * @brief Gets a mutable reference to the version of the catalog to be published.
   *
   * @return The mutable reference to the version of the catalog to be published.
   */
  boost::optional<int64_t>& GetMutableCatalogVersion() {
    return catalog_version_;
  }

  /**
   * @brief Sets the catalog version.
   *
   * @param value The catalog version.
   */
  void SetCatalogVersion(const int64_t& value) {
    this->catalog_version_ = value;
  }

  /**
   * @brief Gets the version dependencies.
   *
   * @return The version dependencies.
   */
  const boost::optional<std::vector<VersionDependency>>&
  GetVersionDependencies() const {
    return version_dependencies_;
  }

  /**
   * @brief Gets a mutable reference to the version dependencies.
   *
   * @return The mutable reference to the version dependencies.
   */
  boost::optional<std::vector<VersionDependency>>&
  GetMutableVersionDependencies() {
    return version_dependencies_;
  }

  /**
   * @brief Sets the version dependencies.
   *
   * @param value The version dependencies.
   */
  void SetVersionDependencies(const std::vector<VersionDependency>& value) {
    this->version_dependencies_ = value;
  }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
