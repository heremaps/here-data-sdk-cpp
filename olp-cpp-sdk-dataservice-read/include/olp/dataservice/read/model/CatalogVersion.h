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

#include <string>
#include <utility>

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/// Represents catalog with the version.
class DATASERVICE_READ_API CatalogVersion final {
 public:
  /**
   * @brief Gets the HRN of the catalog.
   *
   * @return The HRN of the catalog.
   */
  const std::string& GetHrn() const { return hrn_; }

  /**
   * @brief Sets the HRN of the catalog.
   *
   * @param hrn The HRN of the catalog.
   */
  void SetHrn(std::string hrn) { hrn_ = std::move(hrn); }

  /**
   * @brief Gets the version of the catalog.
   *
   * @return The version of the catalog.
   */
  int64_t GetVersion() const { return version_; }

  /**
   * @brief Sets the version of the dependent catalog.
   *
   * @param version The version of the catalog.
   */
  void SetVersion(int64_t version) { version_ = version; }

 private:
  std::string hrn_;
  int64_t version_{0};
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
