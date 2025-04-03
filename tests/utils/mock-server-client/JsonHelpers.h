/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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

#include <boost/json/parse.hpp>
#include <string>

#include <boost/json/value.hpp>

namespace mockserver {

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
  value.emplace_string() =
      boost::json::string_view{reinterpret_cast<char*>(x->data()), x->size()};
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
  for (const auto& entry : x) {
    boost::json::value item_value;
    to_json(entry.second, item_value);
    object.emplace(entry.first, std::move(item_value));
  }
}

template <typename T>
inline void to_json(const std::vector<T>& x, boost::json::value& value) {
  auto& array = value.emplace_array();
  for (const auto& entry : x) {
    boost::json::value item_value;
    to_json(entry, item_value);
    array.emplace_back(std::move(item_value));
  }
}

template <typename T>
inline void serialize(const std::string& key, const T& x,
                      boost::json::value& value) {
  boost::json::value item_value;
  to_json(x, item_value);
  if (!item_value.is_null()) {
    value.emplace_object().emplace(key, std::move(item_value));
  }
}

inline void from_json(const boost::json::value& value, int32_t& x) {
  x = static_cast<int32_t>(value.to_number<int64_t>());
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
  if (auto* found_value = value.as_object().if_contains(name)) {
    from_json(*found_value, result);
  }
  return result;
}

template <typename T>
inline T parse(std::stringstream& json_stream) {
  boost::json::error_code ec;
  auto doc = boost::json::parse(json_stream, ec);
  T result{};
  if (doc.is_object() || doc.is_array()) {
    from_json(doc, result);
  }
  return result;
}

}  // namespace mockserver
