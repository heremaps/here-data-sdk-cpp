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

#include "VersionInfosParser.h"

#include <map>
#include <vector>

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
namespace model = olp::dataservice::read::model;

void from_json(const rapidjson::Value& value, model::VersionDependency& x) {
  x.SetHrn(parse<std::string>(value, "hrn"));
  x.SetVersion(parse<int64_t>(value, "version"));
  x.SetDirect(parse<bool>(value, "direct"));
}

void from_json(const rapidjson::Value& value, model::VersionInfo& x) {
  x.SetDependencies(
      parse<std::vector<model::VersionDependency>>(value, "dependencies"));
  x.SetTimestamp(parse<int64_t>(value, "timestamp"));
  x.SetVersion(parse<int64_t>(value, "version"));
  x.SetPartitionCounts(
      parse<std::map<std::string, int64_t>>(value, "partitionCounts"));
}

void from_json(const rapidjson::Value& value, model::VersionInfos& x) {
  x.SetVersions(parse<std::vector<model::VersionInfo>>(value, "versions"));
}

}  // namespace parser
}  // namespace olp
