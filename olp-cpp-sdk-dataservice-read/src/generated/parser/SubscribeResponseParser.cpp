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

#include "SubscribeResponseParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
using namespace olp::dataservice::read;

void from_json(const rapidjson::Value& value, model::SubscribeResponse& x) {
  x.SetNodeBaseURL(parse<std::string>(value, "nodeBaseURL"));
  x.SetSubscriptionId(parse<std::string>(value, "subscriptionId"));
}

}  // namespace parser
}  // namespace olp
