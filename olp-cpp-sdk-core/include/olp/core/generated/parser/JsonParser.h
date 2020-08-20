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

#pragma once

#include <sstream>
#include <string>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include "ParserWrapper.h"

namespace olp {
namespace parser {
template <typename T>
inline T parse(const std::string& json, bool& res) {
  rapidjson::Document doc;

  doc.Parse(json.c_str());
  T result{};
  res = false;
  if (doc.IsObject() || doc.IsArray()) {
    from_json(doc, result);
    res = true;
  }
  return result;
}

template <typename T>
inline T parse(const std::string& from_json) {
  bool res = true;
  return parse<T>(from_json, res);
}

template <typename T>
inline T parse(std::stringstream& json_stream, bool& res) {
  res = false;
  rapidjson::Document doc;
  rapidjson::IStreamWrapper stream(json_stream);
  doc.ParseStream(stream);
  T result{};
  if (doc.IsObject() || doc.IsArray()) {
    from_json(doc, result);
    res = true;
  }
  return result;
}

template <typename T>
inline T parse(std::stringstream& json_stream) {
  bool res = true;
  return parse<T>(json_stream, res);
}

template <typename OutputResult, typename ParsingType, typename ErrorType,
          typename... AdditionalArgs>
OutputResult parse_result(std::stringstream& json_stream,
                          ErrorType error_if_fail,
                          const AdditionalArgs&&... args) {
  bool res = true;
  auto obj = parse<ParsingType>(json_stream, res);

  if (res) {
    return OutputResult(std::move(obj), args...);
  } else {
    return {std::move(error_if_fail)};
  }
}

}  // namespace parser

}  // namespace olp
