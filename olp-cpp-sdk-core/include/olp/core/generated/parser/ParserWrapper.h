/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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
#include <vector>

#include <rapidjson/rapidjson.h>
#include <boost/optional.hpp>

namespace olp {
namespace parser {

inline void from_json(const rapidjson::Value& value, std::string& x) {
  x = value.GetString();
}

inline void from_json(const rapidjson::Value& value, int32_t& x) {
  x = value.GetInt();
}

inline void from_json(const rapidjson::Value& value, int64_t& x) {
  x = value.GetInt64();
}

inline void from_json(const rapidjson::Value& value, double& x) {
  x = value.GetDouble();
}

inline void from_json(const rapidjson::Value& value, bool& x) {
  x = value.GetBool();
}

inline void from_json(const rapidjson::Value& value,
                      std::shared_ptr<std::vector<unsigned char>>& x) {
  std::string s = value.GetString();
  x = std::make_shared<std::vector<unsigned char>>(s.begin(), s.end());
}

template <typename T>
inline void from_json(const rapidjson::Value& value, boost::optional<T>& x) {
  T result = T();
  from_json(value, result);
  x = result;
}

template <typename T>
inline void from_json(const rapidjson::Value& value,
                      std::map<std::string, T>& results) {
  for (rapidjson::Value::ConstMemberIterator itr = value.MemberBegin();
       itr != value.MemberEnd(); ++itr) {
    std::string key;
    from_json(itr->name, key);
    from_json(itr->value, results[key]);
  }
}

template <typename T>
inline void from_json(const rapidjson::Value& value, std::vector<T>& results) {
  for (rapidjson::Value::ConstValueIterator itr = value.Begin();
       itr != value.End(); ++itr) {
    T result;
    from_json(*itr, result);
    results.emplace_back(std::move(result));
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

}  // namespace parser
}  // namespace olp
