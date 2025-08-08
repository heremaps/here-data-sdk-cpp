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

#include "MessagesParser.h"

// clang-format off
#include "generated/parser/StreamOffsetParser.h"
#include <olp/core/generated/parser/ParserWrapper.h>
// clang-format on

namespace olp {
namespace parser {
namespace model = dataservice::read::model;

void from_json(const rapidjson::Value& value, model::Metadata& x) {
  x.SetPartition(parse<std::string>(value, "partition"));
  x.SetChecksum(parse<porting::optional<std::string>>(value, "checksum"));
  x.SetCompressedDataSize(
      parse<porting::optional<int64_t>>(value, "compressedDataSize"));
  x.SetDataSize(parse<porting::optional<int64_t>>(value, "dataSize"));
  x.SetData(parse<model::Data>(value, "data"));
  x.SetDataHandle(parse<porting::optional<std::string>>(value, "dataHandle"));
  x.SetTimestamp(parse<porting::optional<int64_t>>(value, "timestamp"));
}

void from_json(const rapidjson::Value& value, model::Message& x) {
  x.SetMetaData(parse<model::Metadata>(value, "metaData"));
  x.SetOffset(parse<model::StreamOffset>(value, "offset"));
}

void from_json(const rapidjson::Value& value, model::Messages& x) {
  x.SetMessages(parse<std::vector<model::Message>>(value, "messages"));
}

}  // namespace parser
}  // namespace olp
