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
    for (auto &field : addition.GetIndexFields()) {
      auto value = field.second;
      const auto key = rapidjson::StringRef(field.first.c_str());
      if (value->getIndexType() ==
          dataservice::write::model::IndexType::String) {
        auto s = std::static_pointer_cast<
            dataservice::write::model::StringIndexValue>(value);
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
    additionValue.AddMember("fields", indexFields, allocator);
    // TODO: Separate Details Model serializtion into it's own file when needed
    // by another model.
    if (addition.GetMetadata()) {
      rapidjson::Value metadatas(rapidjson::kObjectType);
      for (auto metadata : addition.GetMetadata().get()) {
        auto value = metadata.second;
        auto key = metadata.first.c_str();
        indexFields.AddMember(rapidjson::StringRef(key),
                              rapidjson::StringRef(value.c_str()), allocator);
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
