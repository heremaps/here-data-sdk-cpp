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

#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/CatalogVersion.h>

#include <utility>
#include <vector>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Contains fields required to request compatible versions for
 * the given catalog.
 */
class DATASERVICE_READ_API CompatibleVersionsRequest final {
 public:
  using Dependencies = std::vector<model::CatalogVersion>;

  /**
   * @brief Gets the dependencies for the catalog versions compatibility check.
   *
   * @return The dependencies for the catalog versions compatibility check.
   */
  inline const Dependencies& GetDependencies() const { return dependencies_; }

  /**
   * @brief Sets the dependencies to check the catalog versions for
   * compatibility.
   *
   * @param dependencies The dependencies for the catalog versions
   * compatibility check.
   *
   * @return A reference to the updated `CompatibleVersionsRequest` instance.
   */
  inline CompatibleVersionsRequest& WithDependencies(
      Dependencies dependencies) {
    dependencies_ = std::move(dependencies);
    return *this;
  }

  /**
   * @brief Gets the maximum amount of versions available in the response.
   *
   * @return The maximum amount of versions available in the response.
   */
  inline int32_t GetLimit() const { return limit_; }

  /**
   * @brief Sets the maximum amount of versions available in the
   * response.
   *
   * @param limit The maximum amount of versions available in the
   * response. The default limit is 1.
   *
   * @return A reference to the updated `CompatibleVersionsRequest` instance.
   */
  inline CompatibleVersionsRequest& WithLimit(int32_t limit) {
    limit_ = limit;
    return *this;
  }

 private:
  Dependencies dependencies_;
  int32_t limit_{1};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
