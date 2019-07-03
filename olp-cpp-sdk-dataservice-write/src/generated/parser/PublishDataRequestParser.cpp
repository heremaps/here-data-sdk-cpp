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

#include "PublishDataRequestParser.h"

// clang-format off
#include <olp/core/generated/parser/ParserWrapper.h>
// clang-format on

namespace olp {
namespace parser {
void from_json(const rapidjson::Value& value,
               olp::dataservice::write::model::PublishDataRequest& x) {
  x.WithData(
      (parse<std::shared_ptr<std::vector<unsigned char>>>(value, "data")));

  x.WithLayerId(parse<std::string>(value, "layerId"));

  x.WithTraceId(parse<std::string>(value, "traceId"));

  x.WithBillingTag(parse<std::string>(value, "billingTag"));

  x.WithChecksum(parse<std::string>(value, "checksum"));
}

}  // namespace parser
}  // namespace olp
