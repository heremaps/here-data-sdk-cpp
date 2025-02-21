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
#include <utility>
#include <vector>

#include <boost/json/value.hpp>
#include <boost/optional.hpp>

namespace olp {
namespace serializer {

inline void to_json(const std::string& x, boost::json::value& value) {
  value.emplace_string() = x;
}

inline void to_json(int32_t x, boost::json::value& value) {
  value.emplace_int64() = x;
}

inline void to_json(int64_t x, boost::json::value& value) {
  value.emplace_int64() = x;
}

inline void to_json(double x, boost::json::value& value) {
  value.emplace_double() = x;
}
inline void to_json(bool x, boost::json::value& value) {
  value.emplace_bool() = x;
}

inline void to_json(const std::shared_ptr<std::vector<unsigned char>>& x,
                    boost::json::value& value) {
  value.emplace_string().assign(x->begin(), x->end());
}

template <typename T>
inline void to_json(const boost::optional<T>& x, boost::json::value& value) {
  if (x) {
    to_json(x.get(), value);
  } else {
    value.emplace_null();
  }
}

template <typename T>
inline void to_json(const std::map<std::string, T>& x,
                    boost::json::value& value) {
  auto& object = value.emplace_object();
  for (auto itr = x.begin(); itr != x.end(); ++itr) {
    const auto& key = itr->first;
    boost::json::value item_value;
    to_json(itr->second, item_value);
    object.emplace(key, std::move(item_value));
  }
}

template <typename T>
inline void to_json(const std::vector<T>& x, boost::json::value& value) {
  auto& array = value.emplace_array();
  array.reserve(x.size());
  for (typename std::vector<T>::const_iterator itr = x.begin(); itr != x.end();
       ++itr) {
    boost::json::value item_value;
    to_json(*itr, item_value);
    array.emplace_back(std::move(item_value));
  }
}

template <typename T>
inline void serialize(const std::string& key, const T& x,
                      boost::json::object& value) {
  boost::json::value item_value;
  to_json(x, item_value);
  if (!item_value.is_null()) {
    value.emplace(key, std::move(item_value));
  }
}

}  // namespace serializer
}  // namespace olp
