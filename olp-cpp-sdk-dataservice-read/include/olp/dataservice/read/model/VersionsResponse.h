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
#include <olp/dataservice/read/model/VersionsResponseEntry.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/// Represents a result of the compatible versions request.
class DATASERVICE_READ_API VersionsResponse final {
 public:
  using VersionsResponseEntries = std::vector<VersionsResponseEntry>;

  /**
   * @brief Sets the information on the compatible versions.
   *
   * It contains the version numbers and dependencies for the requested catalog.
   *
   * @param entries The information on the compatible versions.
   */
  void SetVersionResponseEntries(VersionsResponseEntries entries) {
    entries_ = std::move(entries);
  }

  /**
   * @brief Gets the information on the compatible versions.
   *
   * It contains the version numbers and dependencies for the requested catalog.
   *
   * @return The information on the compatible versions.
   */
  const VersionsResponseEntries& GetVersionResponseEntries() const {
    return entries_;
  }

 private:
  VersionsResponseEntries entries_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
