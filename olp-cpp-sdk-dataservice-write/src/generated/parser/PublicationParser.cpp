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

#include "PublicationParser.h"

// clang-format off
#include <generated/parser/DetailsParser.h>
#include <generated/parser/VersionDependencyParser.h>
#include <olp/core/generated/parser/ParserWrapper.h>
// clang-format on

namespace olp {
namespace parser {
void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Publication& x) {
  x.SetId(parse<std::string>(value, "id"));
  x.SetDetails(
      parse<olp::dataservice::write::model::Details>(value, "details"));
  x.SetLayerIds(parse<std::vector<std::string>>(value, "layerIds"));
  x.SetCatalogVersion(parse<int64_t>(value, "catalogVersion"));
  x.SetVersionDependencies(
      parse<std::vector<olp::dataservice::write::model::VersionDependency>>(
          value, "versionDependencies"));
}

}  // namespace parser

}  // namespace olp
