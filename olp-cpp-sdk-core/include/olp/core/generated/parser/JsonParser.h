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

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <boost/json/parse.hpp>

#include "ParserWrapper.h"

namespace olp {
namespace parser {
template <typename T>
inline T parse(const std::string& json) {
  boost::json::error_code ec;
  auto value = boost::json::parse(json, ec);
  T result{};
  if (value.is_object() || value.is_array()) {
    from_json(value, result);
  }
  return result;
}

template <typename T>
inline T parse(std::stringstream& json_stream, bool& res) {
  res = false;
  boost::json::error_code ec;
  auto value = boost::json::parse(json_stream, ec);
  T result{};
  if (value.is_object() || value.is_array()) {
    from_json(value, result);
    res = true;
  }
  return result;
}

template <typename T>
inline T parse(std::stringstream& json_stream) {
  bool res = true;
  return parse<T>(json_stream, res);
}

template <typename T>
inline T parse(const std::shared_ptr<std::vector<unsigned char>>& json_bytes) {
  boost::json::string_view json(reinterpret_cast<char*>(json_bytes->data()),
                                json_bytes->size());
  boost::json::error_code ec;
  auto value = boost::json::parse(json, ec);
  T result{};
  if (value.is_object() || value.is_array()) {
    from_json(value, result);
  }
  return result;
}

}  // namespace parser
}  // namespace olp
