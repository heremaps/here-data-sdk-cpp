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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/json/value.hpp>
#include <boost/optional.hpp>

namespace olp {
namespace parser {

inline void from_json(const boost::json::value& value, std::string& x) {
  const auto& str = value.get_string();
  x.assign(str.begin(), str.end());
}

inline void from_json(const boost::json::value& value, int32_t& x) {
  x = static_cast<int32_t>(value.to_number<int64_t>());
}

inline void from_json(const boost::json::value& value, int64_t& x) {
  x = value.to_number<int64_t>();
}

inline void from_json(const boost::json::value& value, double& x) {
  x = value.to_number<double>();
}

inline void from_json(const boost::json::value& value, bool& x) {
  x = value.get_bool();
}

inline void from_json(const boost::json::value& value,
                      std::shared_ptr<std::vector<unsigned char>>& x) {
  const auto& s = value.get_string();
  x = std::make_shared<std::vector<unsigned char>>(s.begin(), s.end());
}

template <typename T>
inline void from_json(const boost::json::value& value, boost::optional<T>& x) {
  T result = T();
  from_json(value, result);
  x = result;
}

template <typename T>
inline void from_json(const boost::json::value& value,
                      std::map<std::string, T>& results) {
  const auto& object = value.get_object();
  for (const auto& object_value : object) {
    std::string key = object_value.key();
    from_json(object_value.value(), results[key]);
  }
}

template <typename T>
inline void from_json(const boost::json::value& value,
                      std::vector<T>& results) {
  const auto& array = value.get_array();
  for (const auto& array_value : array) {
    T result;
    from_json(array_value, result);
    results.emplace_back(std::move(result));
  }
}

template <typename T>
inline T parse(const boost::json::value& value, const std::string& name) {
  T result = T();
  const auto& object = value.get_object();
  auto itr = object.find(name);
  if (itr != object.end()) {
    from_json(itr->value(), result);
  }
  return result;
}

}  // namespace parser
}  // namespace olp
