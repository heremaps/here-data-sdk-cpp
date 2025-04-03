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

#include "PublishPartitionSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::PublishPartition& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  if (x.GetPartition()) {
    object.emplace("partition", x.GetPartition().get());
  }

  if (x.GetChecksum()) {
    object.emplace("checksum", x.GetChecksum().get());
  }

  if (x.GetCompressedDataSize()) {
    object.emplace("compressedDataSize", x.GetCompressedDataSize().get());
  }

  if (x.GetDataSize()) {
    object.emplace("dataSize", x.GetDataSize().get());
  }

  if (x.GetData()) {
    const auto& data = x.GetData().get();
    auto data_stringref = boost::json::string_view(
        reinterpret_cast<char*>(data->data()), data->size());
    object.emplace("data", data_stringref);
  }

  if (x.GetDataHandle()) {
    object.emplace("dataHandle", x.GetDataHandle().get());
  }

  if (x.GetTimestamp()) {
    object.emplace("timestamp", x.GetTimestamp().get());
  }
}

}  // namespace serializer

}  // namespace olp
