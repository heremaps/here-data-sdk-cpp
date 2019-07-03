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

#include <string>
#include <vector>

#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

/**
 * @brief Model to represent a partition in a layer.
 */
class Partition {
 public:
  Partition() = default;
  virtual ~Partition() = default;

 private:
  boost::optional<std::string> checksum_;
  boost::optional<int64_t> compressed_data_size_;
  std::string data_handle_;
  boost::optional<int64_t> data_size_;
  std::string partition_;
  boost::optional<int64_t> version_;

 public:
  const boost::optional<std::string>& GetChecksum() const { return checksum_; }
  boost::optional<std::string>& GetMutableChecksum() { return checksum_; }
  void SetChecksum(const boost::optional<std::string>& value) {
    this->checksum_ = value;
  }

  const boost::optional<int64_t>& GetCompressedDataSize() const {
    return compressed_data_size_;
  }
  boost::optional<int64_t>& GetMutableCompressedDataSize() {
    return compressed_data_size_;
  }
  void SetCompressedDataSize(const boost::optional<int64_t>& value) {
    this->compressed_data_size_ = value;
  }

  const std::string& GetDataHandle() const { return data_handle_; }
  std::string& GetMutableDataHandle() { return data_handle_; }
  void SetDataHandle(const std::string& value) { this->data_handle_ = value; }

  const boost::optional<int64_t>& GetDataSize() const { return data_size_; }
  boost::optional<int64_t>& GetMutableDataSize() { return data_size_; }
  void SetDataSize(const boost::optional<int64_t>& value) {
    this->data_size_ = value;
  }

  const std::string& GetPartition() const { return partition_; }
  std::string& GetMutablePartition() { return partition_; }
  void SetPartition(const std::string& value) { this->partition_ = value; }

  const boost::optional<int64_t>& GetVersion() const { return version_; }
  boost::optional<int64_t>& GetMutableVersion() { return version_; }
  void SetVersion(const boost::optional<int64_t>& value) {
    this->version_ = value;
  }
};

/**
 * @brief Model to represent a collection of partitions of a layer.
 */
class Partitions {
 public:
  Partitions() = default;
  virtual ~Partitions() = default;

 private:
  std::vector<Partition> partitions_;

 public:
  const std::vector<Partition>& GetPartitions() const { return partitions_; }
  std::vector<Partition>& GetMutablePartitions() { return partitions_; }
  void SetPartitions(const std::vector<Partition>& value) {
    this->partitions_ = value;
  }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
