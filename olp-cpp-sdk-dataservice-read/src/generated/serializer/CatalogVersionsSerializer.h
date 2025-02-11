/*
 * Copyright (C) 2022-2025 HERE Europe B.V.
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

#pragma once

#include <olp/core/generated/serializer/SerializerWrapper.h>
#include <olp/dataservice/read/model/CatalogVersion.h>
#include <boost/json/value.hpp>

namespace olp {
namespace serializer {

inline void to_json(const dataservice::read::model::CatalogVersion& x,
                    boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("hrn", x.GetHrn(), object);
  serialize("version", x.GetVersion(), object);
}

template <>
inline void to_json<dataservice::read::model::CatalogVersion>(
    const std::vector<dataservice::read::model::CatalogVersion>& x,
    boost::json::value& value) {
  auto& object = value.emplace_object();

  boost::json::array array_value;
  for (const auto& version : x) {
    boost::json::value item_value;
    to_json(version, item_value);
    array_value.emplace_back(std::move(item_value));
  }

  object.emplace("dependencies", std::move(array_value));
}

}  // namespace serializer
}  // namespace olp
