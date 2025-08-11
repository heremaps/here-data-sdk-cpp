/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/porting/optional.h>
#include <rapidjson/document.h>

namespace olp {
namespace serializer {

inline void to_json(const std::string& x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetString(rapidjson::StringRef(x.c_str(), x.size()), allocator);
}

inline void to_json(int32_t x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType&) {
  value.SetInt(x);
}

inline void to_json(int64_t x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType&) {
  value.SetInt64(x);
}

inline void to_json(double x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType&) {
  value.SetDouble(x);
}
inline void to_json(bool x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType&) {
  value.SetBool(x);
}

inline void to_json(const std::shared_ptr<std::vector<unsigned char>>& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetString(reinterpret_cast<char*>(x->data()),
                  static_cast<rapidjson::SizeType>(x->size()), allocator);
}

template <typename T>
inline void to_json(const porting::optional<T>& x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  if (x) {
    to_json(*x, value, allocator);
  } else {
    value.SetNull();
  }
}

template <typename T>
inline void to_json(const std::map<std::string, T>& x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  for (auto itr = x.begin(); itr != x.end(); ++itr) {
    const auto& key = itr->first;
    rapidjson::Value item_value;
    to_json(itr->second, item_value, allocator);
    value.AddMember(rapidjson::StringRef(key.c_str(), key.size()),
                    std::move(item_value), allocator);
  }
}

template <typename T>
inline void to_json(const std::vector<T>& x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetArray();
  for (typename std::vector<T>::const_iterator itr = x.begin(); itr != x.end();
       ++itr) {
    rapidjson::Value item_value;
    to_json(*itr, item_value, allocator);
    value.PushBack(std::move(item_value), allocator);
  }
}

template <typename T>
inline void serialize(const std::string& key, const T& x,
                      rapidjson::Value& value,
                      rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value key_value;
  to_json(key, key_value, allocator);
  rapidjson::Value item_value;
  to_json(x, item_value, allocator);
  if (!item_value.IsNull()) {
    value.AddMember(std::move(key_value), std::move(item_value), allocator);
  }
}

}  // namespace serializer
}  // namespace olp
