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

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief Model to represent the version of the catalog metadata. Incremented
 * every time catalog metadata is changed. e.g. by publishing new partitions to
 * durable layers.
 */
class DATASERVICE_READ_API VersionResponse {
 public:
  VersionResponse() = default;
  virtual ~VersionResponse() = default;

 private:
  int64_t version_{0};

 public:
  const int64_t& GetVersion() const { return version_; }
  int64_t& GetMutableVersion() { return version_; }
  void SetVersion(const int64_t& value) { this->version_ = value; }
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
