/*
 * Copyright (C) 2022 HERE Europe B.V.
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

#include <utility>
#include <vector>

#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/CatalogVersion.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/// Represents dependencies for the given catalog version.
class DATASERVICE_READ_API VersionsResponseEntry {
 public:
  using CatalogVersions = std::vector<CatalogVersion>;

  /**
   * @brief Sets the information on the dependencies.
   *
   * @param catalog_versions The information of the dependencies.
   */
  void SetCatalogVersions(CatalogVersions catalog_versions) {
    catalog_versions_ = std::move(catalog_versions);
  }

  /**
   * @brief Gets the information on the compatible versions.
   *
   * @return The information on the compatible versions.
   */
  const CatalogVersions& GetCatalogVersions() const {
    return catalog_versions_;
  }

  /**
   * @brief Gets the version of the catalog.
   *
   * @return The version of the catalog.
   */
  int64_t GetVersion() const { return version_; }

  /**
   * @brief Sets the version of the catalog.
   *
   * @param version The version of the catalog.
   */
  void SetVersion(int64_t version) { version_ = version; }

 private:
  int64_t version_{0};
  CatalogVersions catalog_versions_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
