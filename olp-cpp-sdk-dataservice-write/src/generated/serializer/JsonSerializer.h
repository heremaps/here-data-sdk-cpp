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

#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>

namespace olp {
namespace serializer {
template <typename T>
inline std::string serialize(const T& object) {
  boost::json::value value;
  value.emplace_object();
  to_json(object, value);
  return boost::json::serialize(value);
}

}  // namespace serializer

}  // namespace olp
