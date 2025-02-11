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
#include <boost/json/serializer.hpp>
#include <boost/json/value.hpp>

#include "ByteVectorBuffer.h"

namespace olp {
namespace serializer {
template <typename T>
inline std::string serialize(const T& object) {
  boost::json::value value;
  to_json(object, value);
  return boost::json::serialize(value);
}

template <typename T>
inline ByteVectorBuffer::Buffer serialize_bytes(const T& object) {
  boost::json::value value;
  to_json(object, value);

  boost::json::serializer serializer;
  serializer.reset(&value);

  auto buffer = std::make_shared<std::vector<unsigned char>>();

  while (!serializer.done()) {
    char temp[4096];
    auto result = serializer.read(temp);
    buffer->insert(buffer->end(), temp, temp + result.size());
  }

  return buffer;
}

}  // namespace serializer

}  // namespace olp
