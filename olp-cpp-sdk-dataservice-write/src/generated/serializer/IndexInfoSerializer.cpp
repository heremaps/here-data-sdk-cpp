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

#include "IndexInfoSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::Index &x, rapidjson::Value &value,
             rapidjson::Document::AllocatorType &allocator) {
  rapidjson::Value jsonValue(rapidjson::kObjectType);
  value.SetArray();
  jsonValue.AddMember("id", rapidjson::StringRef(x.GetId().c_str()), allocator);

  rapidjson::Value indexFields(rapidjson::kObjectType);
  for (auto &field : x.GetIndexFields()) {
    auto value = field.second;
    const auto key = rapidjson::StringRef(field.first.c_str());
    if (value->getIndexType() == dataservice::write::model::IndexType::String) {
      auto s =
          std::static_pointer_cast<dataservice::write::model::StringIndexValue>(
              value);
      rapidjson::Value str_val;
      str_val.SetString(s->GetValue().c_str(), (int)s->GetValue().size(),
                        allocator);
      indexFields.AddMember(key, str_val, allocator);

    } else if (value->getIndexType() ==
               dataservice::write::model::IndexType::Int) {
      auto s =
          std::static_pointer_cast<dataservice::write::model::IntIndexValue>(
              value);
      indexFields.AddMember(key, s->GetValue(), allocator);
    } else if (value->getIndexType() ==
               dataservice::write::model::IndexType::Bool) {
      auto s = std::static_pointer_cast<
          dataservice::write::model::BooleanIndexValue>(value);
      indexFields.AddMember(key, s->GetValue(), allocator);
    } else if (value->getIndexType() ==
               dataservice::write::model::IndexType::Heretile) {
      auto s = std::static_pointer_cast<
          dataservice::write::model::HereTileIndexValue>(value);
      indexFields.AddMember(key, s->GetValue(), allocator);
    } else if (value->getIndexType() ==
               dataservice::write::model::IndexType::TimeWindow) {
      auto s = std::static_pointer_cast<
          dataservice::write::model::TimeWindowIndexValue>(value);
      indexFields.AddMember(key, s->GetValue(), allocator);
    }
  }
  jsonValue.AddMember("fields", indexFields, allocator);
  // TODO: Separate Metadata Model serialization into its own file when needed
  // by another model.
  if (x.GetMetadata()) {
    rapidjson::Value metadatas(rapidjson::kObjectType);
    for (auto metadata : x.GetMetadata().get()) {
      auto value = metadata.second;
      auto key = metadata.first.c_str();
      indexFields.AddMember(rapidjson::StringRef(key),
                            rapidjson::StringRef(value.c_str()), allocator);
    }
  }

  if (x.GetCheckSum()) {
    jsonValue.AddMember("checksum",
                        rapidjson::StringRef(x.GetCheckSum().get().c_str()),
                        allocator);
  }

  if (x.GetSize()) {
    jsonValue.AddMember("size", x.GetSize().get(), allocator);
  }
  value.PushBack(jsonValue, allocator);
}

}  // namespace serializer

}  // namespace olp
