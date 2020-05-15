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

#include <string>
#include <vector>

namespace olp {
namespace dataservice {
namespace read {

class BlobDataWriter {
 public:
  explicit BlobDataWriter(std::vector<unsigned char>& data) : data_(data) {}

  template <class T>
  bool Write(const T& value) {
    if (write_offset_ + sizeof(T) > data_.size()) {
      return false;
    }
    memcpy(&data_[write_offset_], &value, sizeof(T));
    write_offset_ += sizeof(T);
    return true;
  }

  size_t GetOffset() { return write_offset_; }
  void SetOffset(size_t offset) { write_offset_ = offset; }

 private:
  size_t write_offset_ = 0;
  std::vector<unsigned char>& data_;
};

template <>
bool BlobDataWriter::Write<std::string>(const std::string& value) {
  if (write_offset_ + value.size() + 1 > data_.size()) {
    return false;
  }

  memcpy(&data_[write_offset_], value.c_str(), value.size() + 1);
  write_offset_ += value.size() + 1;
  return true;
}
}  // namespace read
}  // namespace dataservice
}  // namespace olp
