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

#include <string>
#include <vector>

namespace olp {
namespace dataservice {
namespace read {

class BlobDataReader {
 public:
  explicit BlobDataReader(const std::vector<unsigned char>& data)
      : data_(data) {}

  template <class T>
  bool Read(T& value) {
    if (read_offset_ + sizeof(T) > data_.size()) {
      return false;
    }
    memcpy(&value, &data_[read_offset_], sizeof(T));
    read_offset_ += sizeof(T);
    return true;
  }

  template <class T>
  bool Skip() {
    if (read_offset_ + sizeof(T) > data_.size()) {
      return false;
    }
    read_offset_ += sizeof(T);
    return true;
  }

  size_t GetOffset() const { return read_offset_; }
  void SetOffset(const size_t offset) { read_offset_ = offset; }

 private:
  size_t read_offset_ = 0;
  const std::vector<unsigned char>& data_;
};

template <>
inline bool BlobDataReader::Read<std::string>(std::string& value) {
  size_t end = std::numeric_limits<size_t>::max();
  for (size_t i = read_offset_; i < data_.size(); ++i) {
    if (data_[i] == 0) {
      end = i;
      break;
    }
  }
  if (end == std::numeric_limits<size_t>::max()) {
    return false;
  }

  value = data_[read_offset_] == 0
              ? std::string()
              : std::string(&data_[read_offset_], &data_[end]);
  read_offset_ = end + 1;
  return true;
}

template <>
inline bool BlobDataReader::Skip<std::string>() {
  size_t end = std::numeric_limits<size_t>::max();
  for (size_t i = read_offset_; i < data_.size(); ++i) {
    if (data_[i] == 0) {
      end = i;
      break;
    }
  }
  if (end == std::numeric_limits<size_t>::max()) {
    return false;
  }

  read_offset_ = end + 1;
  return true;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
