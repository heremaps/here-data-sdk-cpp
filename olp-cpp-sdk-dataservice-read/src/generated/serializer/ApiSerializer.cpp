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

#include <boost/json/value.hpp>

#include "ApiSerializer.h"

#include <olp/core/generated/serializer/SerializerWrapper.h>

namespace olp {
namespace serializer {
void to_json(const dataservice::read::model::Api& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("api", x.GetApi(), object);
  serialize("version", x.GetVersion(), object);
  serialize("baseURL", x.GetBaseUrl(), object);
  serialize("parameters", x.GetParameters(), object);
}
}  // namespace serializer
}  // namespace olp
