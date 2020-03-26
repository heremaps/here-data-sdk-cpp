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

#include "UpdateIndexRequestSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::UpdateIndexRequest &x,
             rapidjson::Value &value,
             rapidjson::Document::AllocatorType &allocator) {
  rapidjson::Value additions(rapidjson::kArrayType);
  for (auto &addition : x.GetIndexAdditions()) {
    rapidjson::Value additionValue(rapidjson::kObjectType);
    additionValue.AddMember(
        "id", rapidjson::StringRef(addition.GetId().c_str()), allocator);

    rapidjson::Value indexFields(rapidjson::kObjectType);
    for (const auto &field_pair : addition.GetIndexFields()) {
      using namespace dataservice::write::model;
      const auto &field = field_pair.second;
      const auto key = rapidjson::StringRef(field_pair.first.c_str());
      auto index_type = field->getIndexType();
      if (index_type == IndexType::String) {
        auto s = std::static_pointer_cast<StringIndexValue>(field);
        rapidjson::Value str_val;
        const auto &str = s->GetValue();
        str_val.SetString(str.c_str(),
                          static_cast<rapidjson::SizeType>(str.size()),
                          allocator);
        indexFields.AddMember(key, str_val, allocator);
      } else if (index_type == IndexType::Int) {
        auto s = std::static_pointer_cast<IntIndexValue>(field);
        indexFields.AddMember(key, s->GetValue(), allocator);
      } else if (index_type == IndexType::Bool) {
        auto s = std::static_pointer_cast<BooleanIndexValue>(field);
        indexFields.AddMember(key, s->GetValue(), allocator);
      } else if (index_type == IndexType::Heretile) {
        auto s = std::static_pointer_cast<HereTileIndexValue>(field);
        indexFields.AddMember(key, s->GetValue(), allocator);
      } else if (index_type == IndexType::TimeWindow) {
        auto s = std::static_pointer_cast<TimeWindowIndexValue>(field);
        indexFields.AddMember(key, s->GetValue(), allocator);
      }
    }
    additionValue.AddMember("fields", indexFields, allocator);
    // TODO: Separate Details Model serializtion into it's own file when needed
    // by another model.
    if (addition.GetMetadata()) {
      rapidjson::Value metadatas(rapidjson::kObjectType);
      for (const auto& metadata : addition.GetMetadata().get()) {
        const auto& metadata_value = metadata.second;
        const auto& metadata_key = metadata.first.c_str();
        indexFields.AddMember(
            rapidjson::StringRef(metadata_key),
            rapidjson::StringRef(metadata_value.c_str(), metadata_value.size()),
            allocator);
      }
    }

    if (addition.GetCheckSum()) {
      additionValue.AddMember(
          "checksum",
          rapidjson::StringRef(addition.GetCheckSum().get().c_str()),
          allocator);
    }

    if (addition.GetSize()) {
      additionValue.AddMember("size", addition.GetSize().get(), allocator);
    }

    additions.PushBack(additionValue, allocator);
  }
  value.AddMember("additions", additions, allocator);
  rapidjson::Value removals(rapidjson::kArrayType);
  for (auto &removal : x.GetIndexRemovals()) {
    removals.PushBack(rapidjson::StringRef(removal.c_str()), allocator);
  }
  value.AddMember("removals", removals, allocator);
}

}  // namespace serializer

}  // namespace olp
