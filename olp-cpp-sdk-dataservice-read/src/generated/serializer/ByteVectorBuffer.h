/*
 * Copyright (C) 2024 HERE Europe B.V.
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
#include <vector>

namespace olp {
namespace serializer {

class ByteVectorBuffer {
 public:
  using Ch = char;
  using Buffer = std::shared_ptr<std::vector<unsigned char>>;

  explicit ByteVectorBuffer(const size_t capacity = kDefaultCapacity)
      : buffer_(std::make_shared<Buffer::element_type>()) {
    buffer_->reserve(capacity);
  }

  void Put(Ch c) { buffer_->emplace_back(c); }
  void Pop(size_t count) { buffer_->resize(buffer_->size() - count); }

  void Flush() {}
  void Clear() { buffer_->clear(); }
  void ShrinkToFit() { buffer_->shrink_to_fit(); }

  void Reserve(size_t count) {
    if (buffer_->capacity() < count) {
      buffer_->reserve(buffer_->size() + count);
    };
  }
  Ch* Push(size_t count) {
    Reserve(count);
    return reinterpret_cast<Ch*>(buffer_->data());
  }

  Buffer GetBuffer() { return buffer_; }

 private:
  // Using the same value as rapidjson
  static constexpr auto kDefaultCapacity = 256u;
  Buffer buffer_;
};

inline void PutReserve(ByteVectorBuffer& stream, size_t count) {
  stream.Reserve(count);
}

inline void PutUnsafe(ByteVectorBuffer& stream, ByteVectorBuffer::Ch c) {
  stream.Put(c);
}
}  // namespace serializer
}  // namespace olp
