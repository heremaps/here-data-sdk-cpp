/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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
#include <string>
#include <vector>

#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

class PublishPartition {
 public:
  PublishPartition() = default;
  PublishPartition(const PublishPartition&) = default;
  PublishPartition(PublishPartition&&) = default;
  PublishPartition& operator=(const PublishPartition&) = default;
  PublishPartition& operator=(PublishPartition&&) = default;
  virtual ~PublishPartition() = default;

 private:
  boost::optional<std::string> partition_;
  boost::optional<std::string> checksum_;
  boost::optional<int64_t> compressed_data_size_;
  boost::optional<int64_t> data_size_;
  std::shared_ptr<std::vector<unsigned char>> data_;
  boost::optional<std::string> data_handle_;
  boost::optional<int64_t> timestamp_;

 public:
  const boost::optional<std::string>& GetPartition() const {
    return partition_;
  }
  boost::optional<std::string>& GetMutablePartition() { return partition_; }
  void SetPartition(const std::string& value) { this->partition_ = value; }

  const boost::optional<std::string>& GetChecksum() const { return checksum_; }
  boost::optional<std::string>& GetMutableChecksum() { return checksum_; }
  void SetChecksum(const std::string& value) { this->checksum_ = value; }

  const boost::optional<int64_t>& GetCompressedDataSize() const {
    return compressed_data_size_;
  }
  boost::optional<int64_t>& GetMutableCompressedDataSize() {
    return compressed_data_size_;
  }
  void SetCompressedDataSize(const int64_t& value) {
    this->compressed_data_size_ = value;
  }

  const boost::optional<int64_t>& GetDataSize() const { return data_size_; }
  boost::optional<int64_t>& GetMutableDataSize() { return data_size_; }
  void SetDataSize(const int64_t& value) { this->data_size_ = value; }

  const std::shared_ptr<std::vector<unsigned char>>& GetData() const {
    return data_;
  }
  std::shared_ptr<std::vector<unsigned char>>& GetMutableData() {
    return data_;
  }
  void SetData(const std::shared_ptr<std::vector<unsigned char>>& value) {
    this->data_ = value;
  }

  const boost::optional<std::string>& GetDataHandle() const {
    return data_handle_;
  }
  boost::optional<std::string>& GetMutableDataHandle() { return data_handle_; }
  void SetDataHandle(const std::string& value) { this->data_handle_ = value; }

  const boost::optional<int64_t>& GetTimestamp() const { return timestamp_; }
  boost::optional<int64_t>& GetMutableTimestamp() { return timestamp_; }
  void SetTimestamp(const int64_t& value) { this->timestamp_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
