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

#include "IndexParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
namespace model = dataservice::read::model;

void from_json(const rapidjson::Value& value,
               std::shared_ptr<model::SubQuad>& x) {
  auto quad = std::make_shared<model::SubQuad>();

  quad->SetAdditionalMetadata(
      parse<porting::optional<std::string>>(value, "additionalMetadata"));

  quad->SetChecksum(parse<porting::optional<std::string>>(value, "checksum"));

  quad->SetCompressedDataSize(
      parse<porting::optional<int64_t>>(value, "compressedDataSize"));

  quad->SetDataHandle(parse<std::string>(value, "dataHandle"));

  quad->SetDataSize(parse<porting::optional<int64_t>>(value, "dataSize"));

  quad->SetSubQuadKey(parse<std::string>(value, "subQuadKey"));
  quad->SetVersion(parse<int64_t>(value, "version"));

  x.swap(quad);
}

void from_json(const rapidjson::Value& value,
               std::shared_ptr<model::ParentQuad>& x) {
  auto quad = std::make_shared<model::ParentQuad>();

  quad->SetAdditionalMetadata(
      parse<porting::optional<std::string>>(value, "additionalMetadata"));

  quad->SetChecksum(parse<porting::optional<std::string>>(value, "checksum"));

  quad->SetCompressedDataSize(
      parse<porting::optional<int64_t>>(value, "compressedDataSize"));

  quad->SetDataHandle(parse<std::string>(value, "dataHandle"));

  quad->SetDataSize(parse<porting::optional<int64_t>>(value, "dataSize"));

  quad->SetPartition(parse<std::string>(value, "partition"));
  quad->SetVersion(parse<int64_t>(value, "version"));

  x.swap(quad);
}

void from_json(const rapidjson::Value& value, model::Index& x) {
  x.SetParentQuads(parse<std::vector<std::shared_ptr<model::ParentQuad>>>(
      value, "parentQuads"));
  x.SetSubQuads(
      parse<std::vector<std::shared_ptr<model::SubQuad>>>(value, "subQuads"));
}

}  // namespace parser

}  // namespace olp
