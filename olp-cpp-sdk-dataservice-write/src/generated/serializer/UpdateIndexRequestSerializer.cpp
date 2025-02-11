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

#include "UpdateIndexRequestSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::UpdateIndexRequest &x,
             boost::json::value &value) {
  boost::json::array additions;
  value.emplace_object();
  for (auto &addition : x.GetIndexAdditions()) {
    boost::json::object additionValue;
    additionValue.emplace("id", addition.GetId());

    boost::json::object indexFields;
    for (const auto &field_pair : addition.GetIndexFields()) {
      using namespace dataservice::write::model;
      const auto &field = field_pair.second;
      const auto &key = field_pair.first;
      auto index_type = field->getIndexType();
      if (index_type == IndexType::String) {
        auto s = std::static_pointer_cast<StringIndexValue>(field);
        boost::json::value str_val{s->GetValue()};
        indexFields.emplace(key, std::move(str_val));
      } else if (index_type == IndexType::Int) {
        auto s = std::static_pointer_cast<IntIndexValue>(field);
        indexFields.emplace(key, s->GetValue());
      } else if (index_type == IndexType::Bool) {
        auto s = std::static_pointer_cast<BooleanIndexValue>(field);
        indexFields.emplace(key, s->GetValue());
      } else if (index_type == IndexType::Heretile) {
        auto s = std::static_pointer_cast<HereTileIndexValue>(field);
        indexFields.emplace(key, s->GetValue());
      } else if (index_type == IndexType::TimeWindow) {
        auto s = std::static_pointer_cast<TimeWindowIndexValue>(field);
        indexFields.emplace(key, s->GetValue());
      }
    }
    additionValue.emplace("fields", indexFields);
    // TODO: Separate Details Model serializtion into it's own file when needed
    // by another model.
    if (addition.GetMetadata()) {
      for (const auto &metadata : addition.GetMetadata().get()) {
        const auto &metadata_value = metadata.second;
        const auto &metadata_key = metadata.first;
        indexFields.emplace(metadata_key, metadata_value);
      }
    }

    if (addition.GetCheckSum()) {
      additionValue.emplace("checksum", addition.GetCheckSum().get());
    }

    if (addition.GetSize()) {
      additionValue.emplace("size", addition.GetSize().get());
    }

    additions.emplace_back(additionValue);
  }
  value.as_object().emplace("additions", std::move(additions));
  boost::json::array removals;
  for (auto &removal : x.GetIndexRemovals()) {
    removals.emplace_back(removal);
  }
  value.as_object().emplace("removals", std::move(removals));
}

}  // namespace serializer

}  // namespace olp
