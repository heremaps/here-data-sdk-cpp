/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "ByteVectorBuffer.h"

namespace olp {
namespace serializer {
template <typename T>
inline std::string serialize(const T& object) {
  rapidjson::Document doc;
  auto& allocator = doc.GetAllocator();

  doc.SetObject();
  to_json(object, doc, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

template <typename T>
inline ByteVectorBuffer::Buffer serialize_bytes(const T& object) {
  rapidjson::Document doc;
  auto& allocator = doc.GetAllocator();

  doc.SetObject();
  to_json(object, doc, allocator);

  ByteVectorBuffer buffer;
  rapidjson::Writer<ByteVectorBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetBuffer();
}

}  // namespace serializer

}  // namespace olp
