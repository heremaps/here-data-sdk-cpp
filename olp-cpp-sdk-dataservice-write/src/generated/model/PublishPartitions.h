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

#include <boost/optional.hpp>

#include <generated/model/PublishPartition.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

class PublishPartitions {
 public:
  PublishPartitions() = default;
  virtual ~PublishPartitions() = default;

 private:
  boost::optional<std::vector<PublishPartition>> partitions_;

 public:
  const boost::optional<std::vector<PublishPartition>>& GetPartitions() const {
    return partitions_;
  }
  boost::optional<std::vector<PublishPartition>>& GetMutablePartitions() {
    return partitions_;
  }
  void SetPartitions(const std::vector<PublishPartition>& value) {
    this->partitions_ = value;
  }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
