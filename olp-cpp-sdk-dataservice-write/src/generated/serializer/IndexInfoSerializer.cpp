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

#include "IndexInfoSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::Index &x,
             boost::json::value &value) {
  boost::json::object jsonValue;
  value.emplace_array();
  jsonValue.emplace("id", x.GetId());

  boost::json::object indexFields;
  for (auto &field_pair : x.GetIndexFields()) {
    namespace model = dataservice::write::model;
    const auto &field = field_pair.second;
    const auto &key = field_pair.first;
    auto index_type = field->getIndexType();
    if (index_type == model::IndexType::String) {
      auto s = std::static_pointer_cast<model::StringIndexValue>(field);
      boost::json::string str_val{s->GetValue()};
      indexFields.emplace(key, std::move(str_val));
    } else if (index_type == model::IndexType::Int) {
      auto s = std::static_pointer_cast<model::IntIndexValue>(field);
      indexFields.emplace(key, s->GetValue());
    } else if (index_type == model::IndexType::Bool) {
      auto s = std::static_pointer_cast<model::BooleanIndexValue>(field);
      indexFields.emplace(key, s->GetValue());
    } else if (index_type == model::IndexType::Heretile) {
      auto s = std::static_pointer_cast<model::HereTileIndexValue>(field);
      indexFields.emplace(key, s->GetValue());
    } else if (index_type == model::IndexType::TimeWindow) {
      auto s = std::static_pointer_cast<model::TimeWindowIndexValue>(field);
      indexFields.emplace(key, s->GetValue());
    }
  }
  jsonValue.emplace("fields", std::move(indexFields));
  // TODO: Separate Metadata Model serialization into its own file when needed
  // by another model.
  if (x.GetMetadata()) {
    for (auto &metadata : x.GetMetadata().get()) {
      auto &metadata_value = metadata.second;
      auto &metadata_key = metadata.first;
      indexFields.emplace(metadata_key, metadata_value);
    }
  }

  if (x.GetCheckSum()) {
    jsonValue.emplace("checksum", x.GetCheckSum().get());
  }

  if (x.GetSize()) {
    jsonValue.emplace("size", x.GetSize().get());
  }
  value.as_array().emplace_back(std::move(jsonValue));
}

}  // namespace serializer

}  // namespace olp
