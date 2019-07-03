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

#include "PublishPartitionParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
void from_json(const rapidjson::Value& value,
               olp::dataservice::write::model::PublishPartition& x) {
  x.SetPartition(parse<std::string>(value, "partition"));
  x.SetChecksum(parse<std::string>(value, "checksum"));
  x.SetCompressedDataSize(parse<int64_t>(value, "compressedDataSize"));
  x.SetDataSize(parse<int64_t>(value, "dataSize"));
  x.SetData(parse<std::shared_ptr<std::vector<unsigned char>>>(value, "data"));
  x.SetDataHandle(parse<std::string>(value, "dataHandle"));
  x.SetTimestamp(parse<int64_t>(value, "timestamp"));
}

}  // namespace parser

}  // namespace olp
