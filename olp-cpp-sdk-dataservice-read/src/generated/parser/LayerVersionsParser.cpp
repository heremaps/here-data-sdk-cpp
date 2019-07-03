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

#include "LayerVersionsParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
using namespace olp::dataservice::read;

void from_json(const rapidjson::Value& value, model::LayerVersion& x) {
  x.SetLayer(parse<std::string>(value, "layer"));
  x.SetVersion(parse<int64_t>(value, "version"));
  x.SetTimestamp(parse<int64_t>(value, "timestamp"));
}

void from_json(const rapidjson::Value& value, model::LayerVersions& x) {
  x.SetLayerVersions(
      parse<std::vector<model::LayerVersion>>(value, "layerVersions"));
  x.SetVersion(parse<int64_t>(value, "version"));
}

}  // namespace parser

}  // namespace olp
