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
#include <utility>

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief Represents a catalog version dependency.
 */
class DATASERVICE_READ_API VersionDependency final {
 public:
  /**
   * @brief Gets the direct status.
   *
   * @return The direct status.
   */
  bool GetDirect() const { return direct_; }

  /**
   * @brief Sets the direct status.
   *
   * @param direct status.
   */
  void SetDirect(bool direct) { direct_ = direct; }

  /**
   * @brief Gets the HRN.
   *
   * @return The HRN.
   */
  const std::string& GetHrn() const { return hrn_; }

  /**
   * @brief Sets the HRN.
   *
   * @param The HRN that you want to set.
   */
  void SetHrn(std::string hrn) { hrn_ = std::move(hrn); }

  /**
   * @brief Gets the catalog version.
   *
   * @return The catalog version.
   */
  std::int64_t GetVersion() const { return version_; }

  /**
   * @brief Sets the catalog version.
   *
   * @param The catalog version.
   */
  void SetVersion(std::int64_t version) { version_ = version; }

 private:
  bool direct_{false};
  std::string hrn_;
  std::int64_t version_{0};
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
