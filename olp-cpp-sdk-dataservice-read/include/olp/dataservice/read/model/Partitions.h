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

#include <string>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief A model that represents a partition in a layer.
 */
class DATASERVICE_READ_API Partition {
 public:
  Partition() = default;
  Partition(const Partition&) = default;
  Partition(Partition&&) = default;
  Partition& operator=(const Partition&) = default;
  Partition& operator=(Partition&&) = default;
  virtual ~Partition() = default;

 private:
  boost::optional<std::string> checksum_;
  boost::optional<int64_t> compressed_data_size_;
  std::string data_handle_;
  boost::optional<int64_t> data_size_;
  boost::optional<std::string> crc_;
  std::string partition_;
  boost::optional<int64_t> version_;

 public:
  /**
   * @brief (Optional) Gets the partition checksum.
   *
   * It is only provided to the API calls that explicitly request a checksum and
   * only matches partitions that have a checksum defined. You can request
   * partitions with a specific checksum by using the `additionalFields` query
   * parameter. If you need to compare data sets for this catalog, set this
   * field to match the `SHA-1` checksum of the corresponding data blob.
   * The maximum length of the checksum field is 128 characters.
   *
   * @return The partition checksum.
   */
  const boost::optional<std::string>& GetChecksum() const { return checksum_; }
  /**
   * @brief (Optional) Gets a mutable reference to the partition checksum.
   *
   * @see `GetChecksum` for information on the checksum.
   *
   * @return The mutable reference to the partition checksum.
   */
  boost::optional<std::string>& GetMutableChecksum() { return checksum_; }
  /**
   * @brief (Optional) Sets the partition checksum.
   *
   * @see `GetChecksum` for information on the checksum.
   *
   * @param value The partition checksum.
   */
  void SetChecksum(boost::optional<std::string> value) {
    this->checksum_ = std::move(value);
  }

  /**
   * @brief (Optional) Gets the compressed size of the partition data in bytes
   * when data compression is enabled.
   *
   * It is only provided to the API calls that explicitly request the compressed
   * data size and only matches partitions that have a compressed data size
   * defined. You can request partitions with a specific compressed data size by
   * using the `additionalFields` query parameter.
   *
   * @return The compressed size of the partition data.
   */
  const boost::optional<int64_t>& GetCompressedDataSize() const {
    return compressed_data_size_;
  }
  /**
   * @brief (Optional) Gets a mutable reference to the compressed size of
   * the partition data in bytes when data compression is enabled.
   *
   * @see `GetCompressedDataSize` for information on the compressed size of
   * the partition data.
   *
   * @return The mutable reference to the compressed size of the partition data.
   */
  boost::optional<int64_t>& GetMutableCompressedDataSize() {
    return compressed_data_size_;
  }
  /**
   * @brief (Optional) Sets the compressed size of the partition data.
   *
   * @see `GetCompressedDataSize` for information on the compressed size of
   * the partition data.
   *
   * @param value The compressed size of the partition data.
   */
  void SetCompressedDataSize(boost::optional<int64_t> value) {
    this->compressed_data_size_ = value;
  }

  /**
   * @brief Get the partition data handle.
   *
   * You use the data handle to retrieve the data that relates to this
   * partition. The data handle identifies a specific blob so that you can
   * request the blob contents with the Blob API. When requesting data from
   * the Blob API, you must specify the catalog ID, layer ID, and data
   * handle.
   *
   * @return The partition data handle.
   */
  const std::string& GetDataHandle() const { return data_handle_; }
  /**
   * @brief Gets a mutable reference to the partition data handle.
   *
   * @see `GetPartition` for information on the partition data handle.
   *
   * @return The partition data handle.
   */
  std::string& GetMutableDataHandle() { return data_handle_; }
  /**
   * @brief Sets the partition data handle.
   *
   * @see `GetPartition` for information on the partition data handle.
   *
   * @param value The partition data handle.
   */
  void SetDataHandle(std::string value) {
    this->data_handle_ = std::move(value);
  }

  /**
   * @brief (Optional) Gets the uncompressed size of the partition data in
   * bytes.
   *
   * It is only provided to the API calls that explicitly request the data size
   * and only matches partitions that have a data size defined. You can request
   * partitions with a specific data size by using the `additionalFields` query
   * parameter.
   *
   * @return The uncompressed size of the partition data
   */
  const boost::optional<int64_t>& GetDataSize() const { return data_size_; }
  /**
   * @brief (Optional) Gets a mutable reference to the uncompressed size of
   * the partition data in bytes.
   *
   * @see `GetDataSize` for information on the uncompressed size of
   * the partition data.
   *
   * @return The mutable reference to the uncompressed size of the partition
   * data.
   */
  boost::optional<int64_t>& GetMutableDataSize() { return data_size_; }
  /**
   * @brief (Optional) Sets the uncompressed size of the partition data.
   *
   * @see `GetDataSize` for information on the uncompressed size of
   * the partition data.
   *
   * @param value The uncompressed size of the partition data.
   */
  void SetDataSize(boost::optional<int64_t> value) { this->data_size_ = value; }

  /**
   * Optional value for the CRC of the partition data in bytes.
   *
   * The response only includes the data size if you specify crc in the
   * additionalFields query parameter, and if crc was specified in the partition
   * metadata when it was published.
   *
   * @return The partition CRC.
   */
  const boost::optional<std::string>& GetCrc() const { return crc_; }
  /**
   * @brief (Optional) Gets a mutable reference to the partition crc.
   *
   * @see `GetCrc` for information on the crc.
   *
   * @return The mutable reference to the partition crc.
   */
  boost::optional<std::string>& GetMutableCrc() { return crc_; }
  /**
   * @brief (Optional) Sets the partition crc.
   *
   * @see `GetCrc` for information on the crc.
   *
   * @param value The partition crc.
   */
  void SetCrc(boost::optional<std::string> value) {
    this->crc_ = std::move(value);
  }

  /**
   * @brief Gets the partition key.
   *
   * It is a unique key for a partition within a layer. If the layer
   * partitioning scheme is HERE Tile, the partition key is equivalent to
   * the tile key. The partition key cannot be empty. The maximum length of
   * the partition key is 500 characters.
   *
   * @return The partition key.
   */
  const std::string& GetPartition() const { return partition_; }
  /**
   * @brief Gets a mutable reference to the partition key.
   *
   * @see `GetPartition` for information on the partition key.
   *
   * @return The mutable reference to the partition key.
   */
  std::string& GetMutablePartition() { return partition_; }
  /**
   * @brief Sets the partition key.
   *
   * @see `GetPartition` for information on the partition key.
   *
   * @param value The partition key.
   */
  void SetPartition(std::string value) { this->partition_ = std::move(value); }

  /**
   * @brief (Optional) Gets the version of the catalog when this partition was
   * last changed.
   *
   * It is only provided for active versioned partitions.
   *
   * @note For volatile partitions, the version is always -1.
   *
   * @return The version of the catalog when this partition was last changed.
   */
  const boost::optional<int64_t>& GetVersion() const { return version_; }
  /**
   * @brief (Optional) Gets a mutable reference to the version of the catalog
   * when this partition was last changed.
   *
   * @see `GetVersion` for information on partition versions.
   *
   * @return The mutable reference to the version of the catalog when this
   * partition was last changed.
   */
  boost::optional<int64_t>& GetMutableVersion() { return version_; }
  /**
   * @brief (Optional) Sets the partition version.
   *
   * @see `GetVersion` for information on partition versions.
   *
   * @param value The version of the catalog when this partition was last
   * changed.
   */
  void SetVersion(boost::optional<int64_t> value) { this->version_ = value; }
};

/**
 * @brief A model that represents a collection of layer partitions.
 */
class Partitions {
 public:
  Partitions() = default;
  Partitions(const Partitions&) = default;
  Partitions(Partitions&&) = default;
  Partitions& operator=(const Partitions&) = default;
  Partitions& operator=(Partitions&&) = default;
  virtual ~Partitions() = default;

 private:
  std::vector<Partition> partitions_;

 public:
  /**
   * @brief Gets the list of partitions for the given layer and layer version.
   *
   * @return The list of partitions.
   */
  const std::vector<Partition>& GetPartitions() const { return partitions_; }
  /**
   * @brief Gets a mutable reference to the list of partitions for the given
   * layer and layer version.
   *
   * @return The mutable reference to the list of partitions.
   */
  std::vector<Partition>& GetMutablePartitions() { return partitions_; }
  /**
   * @brief Sets the list of partitions.
   *
   * @param value The list of partitions for the given layer and layer version.
   */
  void SetPartitions(std::vector<Partition> value) {
    this->partitions_ = std::move(value);
  }
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
