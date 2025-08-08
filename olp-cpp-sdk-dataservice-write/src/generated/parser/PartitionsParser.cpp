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

#include "PartitionsParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
namespace model = dataservice::write::model;

void from_json(const rapidjson::Value& value, model::Partition& x) {
  x.SetChecksum(parse<porting::optional<std::string>>(value, "checksum"));
  x.SetCompressedDataSize(
      parse<porting::optional<int64_t>>(value, "compressedDataSize"));
  x.SetDataHandle(parse<std::string>(value, "dataHandle"));
  x.SetDataSize(parse<porting::optional<int64_t>>(value, "dataSize"));
  x.SetPartition(parse<std::string>(value, "partition"));
  x.SetVersion(parse<porting::optional<int64_t>>(value, "version"));
}

void from_json(const rapidjson::Value& value, model::Partitions& x) {
  x.SetPartitions(parse<std::vector<model::Partition>>(value, "partitions"));
}

}  // namespace parser

}  // namespace olp
