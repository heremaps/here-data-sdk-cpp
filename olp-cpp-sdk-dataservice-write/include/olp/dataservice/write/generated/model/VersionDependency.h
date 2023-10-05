/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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
#include <utility>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

/// Represents catalog and version dependencies.
class DATASERVICE_WRITE_API VersionDependency {
 public:
  VersionDependency() = default;
  VersionDependency(const VersionDependency&) = default;
  VersionDependency(VersionDependency&&) = default;
  VersionDependency& operator=(const VersionDependency&) = default;
  VersionDependency& operator=(VersionDependency&&) = default;
  virtual ~VersionDependency() = default;

  /**
   * @brief Creates the `VersionDependency` instance.
   *
   * @param direct Specifies if the dependency is direct.
   * @param hrn The catalog HRN.
   * @param version The catalog version.
   */
  VersionDependency(bool direct, std::string hrn, int64_t version)
      : direct_(direct), hrn_(std::move(hrn)), version_(version) {}

 private:
  bool direct_{false};
  std::string hrn_;
  int64_t version_{0};

 public:
  /**
   * @brief Gets the direct dependency status.
   *
   * @return The direct dependency status.
   */
  const bool& GetDirect() const { return direct_; }

  /**
   * @brief Gets a mutable reference to the direct dependency status.
   *
   * @return The mutable reference to the direct dependency status.
   */
  bool& GetMutableDirect() { return direct_; }

  /**
   * @brief Sets the direct dependency status.
   *
   * @param value The direct dependency status.
   */
  void SetDirect(const bool& value) { this->direct_ = value; }

  /**
   * @brief Gets the catalog HRN.
   *
   * @return The catalog HRN.
   */
  const std::string& GetHrn() const { return hrn_; }

  /**
   * @brief Gets a mutable reference to the catalog HRN.
   *
   * @return The mutable reference to the catalog HRN.
   */
  std::string& GetMutableHrn() { return hrn_; }

  /**
   * @brief Sets the catalog HRN.
   *
   * @param value The catalog HRN.
   */
  void SetHrn(const std::string& value) { this->hrn_ = value; }

  /**
   * @brief Gets the catalog version.
   *
   * @return The catalog version.
   */
  const int64_t& GetVersion() const { return version_; }

  /**
   * @brief Gets a mutable reference to the catalog version.
   *
   * @return The mutable reference to the catalog version.
   */
  int64_t& GetMutableVersion() { return version_; }

  /**
   * @brief Sets the catalog version.
   *
   * @param value The catalog version.
   */
  void SetVersion(const int64_t& value) { this->version_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
