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

#include "PublishPartitionsSerializer.h"

#include "PublishPartitionSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::PublishPartitions& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  if (x.GetPartitions()) {
    rapidjson::Value partitions(rapidjson::kArrayType);
    for (auto& partition : x.GetPartitions().get()) {
      rapidjson::Value partition_value(rapidjson::kObjectType);
      to_json(partition, partition_value, allocator);
      partitions.PushBack(partition_value, allocator);
    }
    value.AddMember("partitions", partitions, allocator);
  }
}

}  // namespace serializer

}  // namespace olp
