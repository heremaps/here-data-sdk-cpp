/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <string>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace mockserver {

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
inline void to_json(const olp::porting::optional<T>& x, rapidjson::Value& value,
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

inline void from_json(const rapidjson::Value& value, int32_t& x) {
  x = value.GetInt();
}

template <typename T>
inline void from_json(const rapidjson::Value& value, std::vector<T>& results) {
  for (rapidjson::Value::ConstValueIterator itr = value.Begin();
       itr != value.End(); ++itr) {
    T result;
    from_json(*itr, result);
    results.push_back(result);
  }
}

template <typename T>
inline T parse(const rapidjson::Value& value, const std::string& name) {
  T result = T();
  rapidjson::Value::ConstMemberIterator itr = value.FindMember(name.c_str());
  if (itr != value.MemberEnd()) {
    from_json(itr->value, result);
  }
  return result;
}

template <typename T>
inline T parse(std::stringstream& json_stream) {
  rapidjson::Document doc;
  rapidjson::IStreamWrapper stream(json_stream);
  doc.ParseStream(stream);
  T result{};
  if (doc.IsObject() || doc.IsArray()) {
    from_json(doc, result);
  }
  return result;
}

}  // namespace mockserver
