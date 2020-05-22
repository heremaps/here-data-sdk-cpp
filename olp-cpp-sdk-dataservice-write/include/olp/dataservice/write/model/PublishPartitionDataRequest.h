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

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include <boost/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/**
 * @brief PublishPartitionRequest used to publish data into
 * a HERE platform versioned/volatile layer.
 */
class DATASERVICE_WRITE_API PublishPartitionDataRequest {
 public:
  PublishPartitionDataRequest() = default;

  /**
   * @return data previously set.
   */
  inline std::shared_ptr<std::vector<unsigned char>> GetData() const {
    return data_;
  }

  /**
   * @param data Content to be uploaded to the HERE platform.
   * @note Required.
   */
  inline PublishPartitionDataRequest& WithData(
      const std::shared_ptr<std::vector<unsigned char>>& data) {
    data_ = data;
    return *this;
  }

  /**
   * @param data Content to be uploaded to the HERE platform.
   * @note Required.
   */
  inline PublishPartitionDataRequest& WithData(
      std::shared_ptr<std::vector<unsigned char>>&& data) {
    data_ = std::move(data);
    return *this;
  }

  /**
   * @return Layer ID previously set.
   */
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @param layer_id Layer ID of the catalog where you want to store the data.
   * The layer type must be Versioned/Volatile.
   * @note Required.
   */
  inline PublishPartitionDataRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @param layer_id Layer ID of the catalog where you want to store the data.
   * The layer type must be Versioned/Volatile.
   * @note Required.
   */
  inline PublishPartitionDataRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @return Partition ID previously set.
   */
  inline const boost::optional<std::string>& GetPartitionId() const {
    return partition_id_;
  }

  /**
   * @param A key that specifies the partition that the content is related to.
   * It is required. If the layer's partitioning scheme is set to heretile, the
   * partition key is the tile key. The maximum length of the partition key is
   * 500 characters.
   * @note Optional.
   */
  inline PublishPartitionDataRequest& WithPartitionId(
      const std::string& partition_id) {
    partition_id_ = partition_id;
    return *this;
  }

  /**
   * @param A key that specifies the partition that the content is related to.
   * It is required. If the layer's partitioning scheme is set to heretile, the
   * partition key is the tile key. The maximum length of the partition key is
   * 500 characters.
   * @note Optional.
   */
  inline PublishPartitionDataRequest& WithPartitionId(
      std::string&& partition_id) {
    partition_id_ = std::move(partition_id);
    return *this;
  }

  /**
   * @return Billing Tag previously set.
   */
  inline const boost::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @note Optional.
   */
  inline PublishPartitionDataRequest& WithBillingTag(
      const std::string& billing_tag) {
    billing_tag_ = billing_tag;
    return *this;
  }

  /**
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @note Optional.
   */
  inline PublishPartitionDataRequest& WithBillingTag(
      std::string&& billing_tag) {
    billing_tag_ = std::move(billing_tag);
    return *this;
  }

  /**
   * @return Checksum previously set.
   */
  inline const boost::optional<std::string>& GetChecksum() const {
    return checksum_;
  }

  /**
   * @param checksum A SHA-256 hash you can provide for
   * validation against the calculated value on the request body hash. This
   * verifies the integrity of your request and prevents modification by a third
   * party.It will be created by the service if not provided. A SHA-256 hash
   * consists of 256 bits or 64 chars.
   * @note Optional.
   */
  inline PublishPartitionDataRequest& WithChecksum(
      const std::string& checksum) {
    checksum_ = checksum;
    return *this;
  }

  /**
   * @param checksum A SHA-256 hash you can provide for
   * validation against the calculated value on the request body hash. This
   * verifies the integrity of your request and prevents modification by a third
   * party.It will be created by the service if not provided. A SHA-256 hash
   * consists of 256 bits or 64 chars.
   * @note Optional.
   */
  inline PublishPartitionDataRequest& WithChecksum(std::string&& checksum) {
    checksum_ = std::move(checksum);
    return *this;
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> data_;

  std::string layer_id_;

  boost::optional<std::string> partition_id_;

  boost::optional<std::string> billing_tag_;

  boost::optional<std::string> checksum_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
