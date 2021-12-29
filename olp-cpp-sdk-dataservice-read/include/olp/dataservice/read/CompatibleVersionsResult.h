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

#include <olp/dataservice/read/CompatibleVersionDependency.h>
#include <olp/dataservice/read/DataServiceReadApi.h>

#include <utility>
#include <vector>

namespace olp {
namespace dataservice {
namespace read {

/// Represents information on a compatible version
/// for the compatible versions request.
struct CompatibleVersionInfo {
  int64_t version;
  std::vector<CompatibleVersionDependency> dependencies;
};

/// Represents a result of the compatible versions request.
class DATASERVICE_READ_API CompatibleVersionsResult {
 public:
  using CompatibleVersionInfos = std::vector<CompatibleVersionInfo>;

  /**
   * @brief Sets the  information on the compatible versions.
   *
   * It contains the version numbers and dependencies for the requested catalog.
   *
   * @param version_infos The information on the compatible versions.
   */
  void SetVersionInfos(CompatibleVersionInfos version_infos) {
    version_infos_ = std::move(version_infos);
  }

  /**
   * @brief Gets the information on the compatible versions.
   *
   * It contains the version numbers and dependencies for the requested catalog.
   *
   * @return The information on the compatible versions.
   */
  const CompatibleVersionInfos& GetVersionInfos() const {
    return version_infos_;
  }

 private:
  CompatibleVersionInfos version_infos_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
