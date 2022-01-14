/*
 * Copyright (C) 2022 HERE Europe B.V.
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

#include "VersionsResponseParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {

using VersionsResponse = olp::dataservice::read::model::VersionsResponse;
using VersionsResponseEntry =
    olp::dataservice::read::model::VersionsResponseEntry;
using CatalogVersion = olp::dataservice::read::model::CatalogVersion;

void from_json(const rapidjson::Value& value, CatalogVersion& x) {
  x.SetHrn(parse<std::string>(value, "hrn"));
  x.SetVersion(parse<int64_t>(value, "version"));
}

void from_json(const rapidjson::Value& value, VersionsResponseEntry& x) {
  x.SetVersion(parse<int64_t>(value, "version"));
  x.SetCatalogVersions(
      parse<std::vector<CatalogVersion>>(value, "sharedDependencies"));
}

void from_json(const rapidjson::Value& value, VersionsResponse& x) {
  x.SetVersionResponseEntries(
      parse<std::vector<VersionsResponseEntry>>(value, "versions"));
}

}  // namespace parser
}  // namespace olp
