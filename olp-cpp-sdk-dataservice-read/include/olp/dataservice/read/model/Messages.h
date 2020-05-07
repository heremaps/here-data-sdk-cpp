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
#include <utility>

#include <boost/optional.hpp>

#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/Data.h>
#include <olp/dataservice/read/model/StreamOffsets.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief Encapsulates information about actual data content.
 */
class DATASERVICE_READ_API Metadata final {
 public:
  /**
   * @brief Gets the partition of this metadata.
   *
   * For more information on partitions, see the [related
   * section](https://developer.here.com/olp/documentation/data-api/data_dev_guide/rest/layers/partitions.html)
   * in the Data API Developer Guide.
   *
   * @return The partition to which this metadata content is related.
   */
  const std::string& GetPartition() const { return partition_; }
  /**
   * @brief Sets the partition ID of this metadata content.
   *
   * @param value The partition ID string. The maximum length is 500 characters.
   */
  void SetPartition(std::string value) { partition_ = std::move(value); }

  /**
   * @brief (Optional) Gets the checksum of this metadata.
   *
   * The algorithm used to calculate the checksum is user-specific. It is not
   * a secure hash. It is used only to detect changes in the content.
   *
   * Examples: `MD5` or `SHA1`
   *
   * @return The checksum of the content.
   */
  const boost::optional<std::string>& GetChecksum() const { return checksum_; }
  /**
   * @brief (Optional) Sets the checksum of this metadata content.
   *
   * @see `GetChecksum` for information on the checksum.
   *
   * @param value The checksum string.
   */
  void SetChecksum(boost::optional<std::string> value) {
    checksum_ = std::move(value);
  }

  /**
   * @brief (Optional) Gets the compressed size of the content (in bytes).
   *
   * It is present only if the \ref `GetDataHandle` method is not empty.
   * Applicable also if `Content-Encoding` is set to `gzip` when uploading and
   * downloading data.
   *
   * @return The compressed size of the content (in bytes).
   */
  const boost::optional<int64_t>& GetCompressedDataSize() const {
    return compressed_data_size_;
  }
  /**
   * @brief (Optional) Sets the compressed size of the content.
   *
   * @see `GetCompressedDataSize` for information on the compressed size of
   * content.
   *
   * @param value The compressed size of the content (in bytes).
   */
  void SetCompressedDataSize(boost::optional<int64_t> value) {
    compressed_data_size_ = value;
  }

  /**
   * @brief (Optional) Gets the nominal size (in bytes) of the content.
   *
   * It is present only if the \ref `GetDataHandle` method is not empty.
   *
   * @note When compression is enabled, this field contains the size of
   * the uncompressed content.
   *
   * @return The data size in bytes.
   */
  const boost::optional<int64_t>& GetDataSize() const { return data_size_; }
  /**
   * @brief (Optional) Sets the nominal size of the content.
   *
   * @see `GetDataSize` for information on the nominal content size.
   *
   * @param value The nominal size of the content in bytes.
   */
  void SetDataSize(boost::optional<int64_t> value) { data_size_ = value; }

  /**
   * @brief Gets the data of this \ref `Metadata` instance.
   *
   * The data represents content published directly in the metadata and encoded
   * in Base64. The size of the content is limited. It is present only if
   * the message size is less than or equal to 1 MB.
   *
   * @return The data of this message represented as a vector of bytes.
   */
  const Data& GetData() const { return data_; }
  /**
   * @brief Sets the data of this content.
   *
   * @param value The data of this content represented as a vector of bytes.
   */
  void SetData(Data value) { data_ = std::move(value); }

  /**
   * @brief (Optional) Gets the data handle created when the content was
   * uploaded.
   *
   * The data handle is a unique identifier that is used to identify this
   * content and retrieve the content at a later stage.
   *
   * Example: `1b2ca68f-d4a0-4379-8120-cd025640510c`
   *
   * @note It is present only if the message size is less than or equal
   * to 1 MB.
   *
   * @return The data handle created when the content was uploaded.
   */
  const boost::optional<std::string>& GetDataHandle() const {
    return data_handle_;
  }

  /**
   * @brief (Optional) Sets the data handle of this content.
   *
   * @see `GetDataHandle` for information on the data handle.
   *
   * @param value The data handle represented as a string.
   */
  void SetDataHandle(boost::optional<std::string> value) {
    data_handle_ = std::move(value);
  }

  /**
   * @brief (Optional) Get the timestamp of the content.
   *
   * This field represents time (in milliseconds since the Unix epoch) when this
   * message was produced to the stream layer.
   *
   * @return The Unix timestamp of the content (in milliseconds).
   */
  const boost::optional<int64_t>& GetTimestamp() const { return timestamp_; }
  /**
   * @brief (Optional) Sets the timestamp of the content.
   *
   * @see `GetTimeStamp` for information on the timestamp.
   *
   * @param value The Unix timestamp of the content (in milliseconds).
   */
  void SetTimestamp(boost::optional<int64_t> value) { timestamp_ = value; }

 private:
  Data data_;
  std::string partition_;
  boost::optional<int64_t> compressed_data_size_;
  boost::optional<int64_t> data_size_;
  boost::optional<int64_t> timestamp_;
  boost::optional<std::string> checksum_;
  boost::optional<std::string> data_handle_;
};

/**
 * @brief Represents a message read from a stream layer.
 */
class DATASERVICE_READ_API Message final {
 public:
  /**
   * @brief Gets the \ref `Metadata` instance of this message.
   *
   * @return The \ref `Metadata` instance.
   */
  const Metadata& GetMetaData() const { return meta_data_; }
  /**
   * @brief Sets the \ref `Metadata` instance of this message.
   *
   * @param value The \ref `Metadata` instance.
   */
  void SetMetaData(Metadata value) { meta_data_ = std::move(value); }

  /**
   * @brief Gets the offset in a specific partition of the stream layer.
   *
   * @return The \ref `StreamOffset` instance.
   */
  const StreamOffset& GetOffset() const { return offset_; }
  /**
   * @brief Sets the \ref `StreamOffset` instance of this message.
   *
   * @param value The \ref `StreamOffset` instance.
   */
  void SetOffset(StreamOffset value) { offset_ = std::move(value); }

  /**
   * @brief Gets the actual content of this message.
   *
   * @return The data content of this message represented as a vector of bytes.
   */
  const Data& GetData() const { return meta_data_.GetData(); }

 private:
  Metadata meta_data_;
  StreamOffset offset_;
};

/**
 * @brief Represents a vector of messages consumed from a stream layer.
 */
class DATASERVICE_READ_API Messages final {
 public:
  /**
   * @brief Gets the vector of messages.
   *
   * @return The vector of messages consumed from \ref `StreamLayerClient`.
   */
  const std::vector<Message>& GetMessages() const { return messages_; }
  /**
   * @brief Sets the vector of messages.
   *
   * @param value The vector of messages consumed from \ref `StreamLayerClient`.
   */
  void SetMessages(std::vector<Message> value) { messages_ = std::move(value); }

 private:
  std::vector<Message> messages_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
