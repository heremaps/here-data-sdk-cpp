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

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

class DATASERVICE_WRITE_API VersionDependency {
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
  const bool& GetDirect() const { return direct_; }
  bool& GetMutableDirect() { return direct_; }
  void SetDirect(const bool& value) { this->direct_ = value; }

  const std::string& GetHrn() const { return hrn_; }
  std::string& GetMutableHrn() { return hrn_; }
  void SetHrn(const std::string& value) { this->hrn_ = value; }

  const int64_t& GetVersion() const { return version_; }
  int64_t& GetMutableVersion() { return version_; }
  void SetVersion(const int64_t& value) { this->version_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
