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

#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <vector>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

class ParentQuad {
public:
  ParentQuad() = default;
  virtual ~ParentQuad() = default;

 private:
  boost::optional<std::string> additional_metadata_;
  boost::optional<std::string> checksum_;
  boost::optional<int64_t> compressed_data_size_;
  std::string data_handle_;
  boost::optional<int64_t> data_size_;
  std::string partition_;
  int64_t version_{0};

 public:
  /**
   * @brief Optional value for the additional metadata specified by the
   * publisher
   **/
  boost::optional<std::string> GetAdditionalMetadata() const {
    return additional_metadata_;
  }

  void SetAdditionalMetadata(boost::optional<std::string> value) {
    additional_metadata_ = value;
  }

  /**
   * @brief The checksum field is optional. The response includes the checksum
   * only if you specify &#x60;checksum&#x60; in the
   * &#x60;additionalFields&#x60; query parameter, and if a checksum was
   * specified in the partition metadata when it was published. You need to use
   * the SHA-1 checksum of the data content if you want data comparison to work
   * for this catalog. The maximum length of the checksum field is 128
   * characters.
   */
  boost::optional<std::string> GetChecksum() const { return checksum_; }

  void SetChecksum(boost::optional<std::string> value) { checksum_ = value; }

  /**
   * @brief Optional value for the size of the compressed partition data in
   *bytes. The response only includes the compressed data size if you specify
   *&#x60;compressedDataSize&#x60; in the &#x60;additionalFields&#x60; query
   *parameter, and if compression is enabled, and if
   *&#x60;compressedDataSize&#x60; was specified in the partition metadata when
   *it was published.
   **/
  boost::optional<int64_t> GetCompressedDataSize() const {
    return compressed_data_size_;
  }
  void SetCompressedDataSize(boost::optional<int64_t> value) {
    compressed_data_size_ = value;
  }

  /**
   * @brief The dataHandle must not contain any characters that are not part of
   * the reserved and unreserved set as defined in RFC3986. This field must not
   * have an empty value. If the dataHandle is not present when comparing two
   * versions, the partition was deleted. The maximum length of dataHandle is
   * 1024 characters.
   **/
  std::string GetDataHandle() const { return data_handle_; }
  void SetDataHandle(std::string value) { data_handle_ = value; }

  /**
   * @brief Optional value for the size of the partition data in bytes. The
   * response only includes the data size if you specify dataSize&#x60; in the
   * &#x60;additionalFields&#x60; query parameter, and if &#x60;dataSize&#x60;
   * was specified in the partition metadata when it was published.
   **/
  boost::optional<int64_t> GetDataSize() const { return data_size_; }
  void SetDataSize(boost::optional<int64_t> value) { data_size_ = value; }

  /**
   * @brief The id of the tile
   */
  std::string GetPartition() const { return partition_; }
  void SetPartition(std::string value) { partition_ = value; }

  /**
   * @brief Version of the catalog when this partition was first published
   */
  int64_t GetVersion() const { return version_; }
  void SetVersion(int64_t value) { version_ = value; }
};

class SubQuad {
 public:
  SubQuad() = default;
  virtual ~SubQuad() = default;

 private:
  boost::optional<std::string> additional_metadata_;
  boost::optional<std::string> checksum_;
  boost::optional<int64_t> compressed_data_size_;
  std::string data_handle_;
  boost::optional<int64_t> data_size_;
  std::string sub_quad_key_;
  int64_t version_{0};

 public:
  /**
   * @brief Optional value for the additional meta-data specified by the
   * publisher.
   **/
  boost::optional<std::string> GetAdditionalMetadata() const {
    return additional_metadata_;
  }

  void SetAdditionalMetadata(boost::optional<std::string> value) {
    additional_metadata_ = value;
  }

  /**
   * @brief
   * The checksum field is optional. The response only includes this
   * information if you specify checksum in the request (using
   * &#x60;additionalFields&#x60; query parameter), and if the request for the
   * commit of the partition  a checksum was provided during commit of the
   * partition. It should be set to the sha1 checksum of the data content if
   * data comparison needs to work for this catalog. The maximum length of
   * checksum field is 128 characters.
   **/
  boost::optional<std::string> GetChecksum() const { return checksum_; }

  void SetChecksum(boost::optional<std::string> value) { checksum_ = value; }

  /**
   * @brief
   * Optional value for the size of the compressed partition data in bytes.
   * Compressed size of the data when using Blob API in the data client with
   * compression enabled to commit to Data Service.  The response only includes
   * this information if you specify the compressedDataSize field in the
   * request, and if the request for the commit of the partition specifies
   * compressedDataSize.
   **/
  boost::optional<int64_t> GetCompressedDataSize() const {
    return compressed_data_size_;
  }
  void SetCompressedDataSize(boost::optional<int64_t> value) {
    compressed_data_size_ = value;
  }

  /**
   * @brief
   * To store data in the directly dataHandle, use the &#x60;data&#x60; URL
   * scheme (RFC 2397). The dataHandle must not contain any characters that are
   * not part of the reserved and unreserved set as defined in RFC3986. This
   * field can have an empty value. When comparing two versions and if the
   * dataHandle is not present, the partition was deleted. The maximum length
   * of dataHandle is 1024 characters.
   **/
  std::string GetDataHandle() const { return data_handle_; }
  void SetDataHandle(std::string value) { data_handle_ = value; }

  /**
   * @brief
   * Optional value for the size of the partition data in bytes. Uncompressed
   * size of the data when using Blob API in the data client with compression
   * enabled or disabled to commit to Data Service. The response only includes
   * this information if you specify the dataSize field in the requested, and
   * if the request for the commit of the partition specifies dataSize.
   **/
  boost::optional<int64_t> GetDataSize() const { return data_size_; }
  void SetDataSize(boost::optional<int64_t> value) { data_size_ = value; }

  /**
   * @brief
   * Variable length string defining the child of the passed in quadKey tile.
   * Depending on the partitioning scheme of the layer, either a
   * &#x60;quadtree&#x60; (deprecated) or a &#x60;heretile&#x60; formatted id
   * from the sub quad will be returned. When the quadKey is referenced in the
   * response (e.g. when depth is 0), subQuadKey is an empty string for
   * &#x60;quadtree&#x60; partitioning, and &#x60;1&#x60; for
   * &#x60;heretile&#x60; partitioning.
   **/
  std::string GetSubQuadKey() const { return sub_quad_key_; }
  void SetSubQuadKey(std::string value) { sub_quad_key_ = value; }

  /**
   * @brief
   * Version of the catalog when this partition was first published.
   **/
  int64_t GetVersion() const { return version_; }
  void SetVersion(int64_t value) { version_ = value; }
};

class Index {
 public:
  Index() = default;
  virtual ~Index() = default;

 private:
  std::vector<std::shared_ptr<ParentQuad>> parent_quads_;
  std::vector<std::shared_ptr<SubQuad>> sub_quads_;

 public:
  /** @brief
   * Result of the index resource call. For each parent tile, one element with
   * the respective parent-quad data is contained in the array.
   **/
  std::vector<std::shared_ptr<ParentQuad>>& GetParentQuads() {
    return parent_quads_;
  }
  void SetParentQuads(std::vector<std::shared_ptr<ParentQuad>> value) {
    parent_quads_ = value;
  }

  /** @brief
   * Result of the index resource call. For each tile that contains data in the
   * requested quadKey, one element with the respective sub-quad data is
   * contained in the array.
   **/
  const std::vector<std::shared_ptr<SubQuad>>& GetSubQuads() const {
    return sub_quads_;
  }
  void SetSubQuads(std::vector<std::shared_ptr<SubQuad>> value) {
    sub_quads_ = value;
  }
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
