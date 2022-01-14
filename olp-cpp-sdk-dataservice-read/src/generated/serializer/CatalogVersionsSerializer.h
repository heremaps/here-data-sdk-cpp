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

#pragma once

#include <olp/core/generated/serializer/SerializerWrapper.h>
#include <olp/dataservice/read/model/CatalogVersion.h>
#include <rapidjson/document.h>

namespace olp {
namespace serializer {

inline void to_json(const dataservice::read::model::CatalogVersion& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("hrn", x.GetHrn(), value, allocator);
  serialize("version", x.GetVersion(), value, allocator);
}

template <>
inline void to_json<dataservice::read::model::CatalogVersion>(
    const std::vector<dataservice::read::model::CatalogVersion>& x,
    rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();

  rapidjson::Value array_value;
  array_value.SetArray();
  for (auto itr = x.begin(); itr != x.end(); ++itr) {
    rapidjson::Value item_value;
    to_json(*itr, item_value, allocator);
    array_value.PushBack(std::move(item_value), allocator);
  }
  value.AddMember("dependencies", std::move(array_value), allocator);
}

}  // namespace serializer
}  // namespace olp
