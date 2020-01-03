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

#include <boost/optional.hpp>
#include <olp/dataservice/read/model/Data.h>
#include "StreamOffset.h"

namespace olp {
namespace dataservice {
namespace read {
namespace model {

class Metadata {
 public:
  Metadata() = default;
  virtual ~Metadata() = default;

  const std::string& GetPartition() const { return partition_; }
  void SetPartition(const std::string& value) { partition_ = value; }

  const boost::optional<std::string>& GetChecksum() const { return checksum_; }
  void SetChecksum(const std::string& value) { checksum_ = value; }

  const boost::optional<int64_t>& GetCompressedDataSize() const {
    return compressed_data_size_;
  }
  void SetCompressedDataSize(int64_t value) { compressed_data_size_ = value; }

  const boost::optional<int64_t>& GetDataSize() const { return data_size_; }
  void SetDataSize(int64_t value) { data_size_ = value; }

  const Data& GetData() const { return data_; }
  void SetData(const Data& value) { data_ = value; }

  const boost::optional<std::string>& GetDataHandle() const {
    return data_handle_;
  }
  void SetDataHandle(const std::string& value) { data_handle_ = value; }

  const boost::optional<int64_t>& GetTimestamp() const { return timestamp_; }
  void SetTimestamp(int64_t value) { timestamp_ = value; }

 private:
  /**
   * @brief A key that specifies which
   * [Partition](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/partitions.html)
   * the content is related to. This is provided by the user while producing to
   * the stream layer. The maximum length of the partition key is 500
   * characters.
   */
  std::string partition_;

  /**
   * @brief The checksum of the content. The algorithm used to calculate the
   * checksum is user specific. Algorithms that can be used are, for example,
   * MD5 or SHA1. This is not a secure hash, it's used only to detect changes in
   * content.
   */
  boost::optional<std::string> checksum_;

  /**
   * @brief The compressed size of the content in bytes. Applicable also when
   * `Content-Encoding` is set to gzip when uploading and downloading data.
   * Note: This will be present only if ‘_dataHandle_’ field is present.
   */
  boost::optional<int64_t> compressed_data_size_;

  /**
   * @brief The nominal size in bytes of the content. When compression is
   * enabled, this is the size of the uncompressed content. Note: This will be
   * present only if the ‘dataHandle’ field is present.
   */
  boost::optional<int64_t> data_size_;

  /**
   * @brief The content published directly in the metadata and encoded in
   * base64. The size of the content is limited. Either `data` or `dataHandle`
   * must be present. Data Handle Example: 1b2ca68f-d4a0-4379-8120-cd025640510c.
   * Note: This will be present only if the message size is less than or equal
   * to 1 MB.
   */
  Data data_;

  /**
   * @brief The handle created when uploading the content. It is used to
   * retrieve the content at a later stage. Either `data` or `dataHandle` must
   * be present. Note: This will be present only if the message size is greater
   * than 1 MB.
   */
  boost::optional<std::string> data_handle_;

  /**
   * @brief The timestamp of the content, in milliseconds since the Unix epoch.
   * This can be provided by the user while producing to the stream layer. Refer
   * to [NewPartition
   * Object](https://developer.here.com/olp/documentation/data-client-library/api_reference_scala/index.html#com.here.platform.data.client.scaladsl.NewPartition).
   * If not provided by the user, this is the timestamp when the message was
   * produced to the stream layer.
   */
  boost::optional<int64_t> timestamp_;
};

class Message {
 public:
  Message() = default;
  virtual ~Message() = default;

  const Metadata& GetMetaData() const { return meta_data_; };
  void SetMetaData(const Metadata& value) { meta_data_ = value; };

  const StreamOffset& GetOffset() const { return offset_; };
  void SetOffset(const StreamOffset& value) { offset_ = value; };

 private:
  Metadata meta_data_;
  StreamOffset offset_;
};

class ConsumeDataResponse {
 public:
  ConsumeDataResponse() = default;
  virtual ~ConsumeDataResponse() = default;

  const std::vector<Message>& GetMessages() const { return messages_; };
  void SetMessages(const std::vector<Message>& value) { messages_ = value; };

 private:
  std::vector<Message> messages_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
