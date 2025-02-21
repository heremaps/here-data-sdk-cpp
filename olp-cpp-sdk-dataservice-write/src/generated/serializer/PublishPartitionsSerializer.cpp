/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include "PublishPartitionsSerializer.h"

#include "PublishPartitionSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::PublishPartitions& x,
             boost::json::value& value) {
  if (x.GetPartitions()) {
    boost::json::array partitions;
    for (auto& partition : x.GetPartitions().get()) {
      boost::json::value partition_value(boost::json::object_kind_t{});
      to_json(partition, partition_value);
      partitions.emplace_back(std::move(partition_value));
    }
    value.as_object().emplace("partitions", std::move(partitions));
  }
}

}  // namespace serializer

}  // namespace olp
