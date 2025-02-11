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

#include "ApiParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Api& x) {
  x.SetApi(parse<std::string>(value, "api"));
  x.SetVersion(parse<std::string>(value, "version"));
  x.SetBaseUrl(parse<std::string>(value, "baseURL"));
  x.SetParameters(
      parse<std::map<std::string, std::string> >(value, "parameters"));
}

}  // namespace parser

}  // namespace olp
