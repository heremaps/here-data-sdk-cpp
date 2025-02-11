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

#include "ResponseOkParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

using namespace olp::dataservice::write::model;

namespace olp {
namespace parser {
void from_json(const boost::json::value& value, TraceID& x) {
  x.SetParentID(parse<std::string>(value, "ParentID"));
  x.SetGeneratedIDs(parse<std::vector<std::string> >(value, "GeneratedIDs"));
}

void from_json(const boost::json::value& value, ResponseOk& x) {
  x.SetTraceID(parse<TraceID>(value, "TraceID"));
}

}  // namespace parser
}  // namespace olp
