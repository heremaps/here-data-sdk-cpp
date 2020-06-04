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
#include <olp/dataservice/read/model/VersionInfo.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief A container for list of version infos.
 */
class DATASERVICE_READ_API VersionInfos final {
 public:
  /**
   * @brief Sets the vector of version infos.
   *
   * @param value The vector of version infos .
   */
  void SetVersionInfos(std::vector<VersionInfo> value) {
    versions_ = std::move(value);
  }

  /**
   * @brief Gets the vector of version infos.
   *
   * @return The vector of version infos.
   */
  const std::vector<VersionInfo>& GetVersionInfos() const { return versions_; }

 private:
  std::vector<VersionInfo> versions_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
