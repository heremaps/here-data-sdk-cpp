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

#include <olp/core/cache/KeyValueCache.h>
#include <rapidjson/document.h>

namespace olp {
namespace parser {
template <typename T>
inline T parse(const cache::KeyValueCache::ValueType& cached_json) {
  rapidjson::Document doc;
  T result{};
  if (!cached_json.empty()) {
    auto* data = static_cast<const void*>(cached_json.data());
    doc.Parse(static_cast<const char*>(data), cached_json.size());
    if (doc.IsObject() || doc.IsArray()) {
      from_json(doc, result);
    }
  }
  return result;
}

}  // namespace parser
}  // namespace olp
