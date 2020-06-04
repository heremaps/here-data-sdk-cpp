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
 * @brief Represents a catalog version dependency
 */
class DATASERVICE_READ_API VersionDependency {
 public:
  VersionDependency() = default;
  virtual ~VersionDependency() = default;
  VersionDependency(bool direct, std::string hrn, int64_t version)
      : direct_(direct), hrn_(std::move(hrn)), version_(version) {}

 private:
  bool direct_{false};
  std::string hrn_;
  int64_t version_{0};

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
   * @brief Gets the hrn.
   *
   * @return The hrn.
   */
  const std::string& GetHrn() const { return hrn_; }

  /**
   * @brief Sets the hrn.
   *
   * @param hrn.
   */
  void SetHrn(const std::string& hrn) { hrn_ = hrn; }

  /**
   * @brief Gets the version.
   *
   * @return The version.
   */
  int64_t GetVersion() const { return version_; }

  /**
   * @brief Sets the version.
   *
   * @param version.
   */
  void SetVersion(int64_t version) { version_ = version; }
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
