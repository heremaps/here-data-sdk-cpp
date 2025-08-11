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
#include <utility>
#include <vector>

#include <olp/core/porting/optional.h>
#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/// Publishes data to a versioned and volatile layer.
class DATASERVICE_WRITE_API PublishPartitionDataRequest {
 public:
  PublishPartitionDataRequest() = default;
  PublishPartitionDataRequest(const PublishPartitionDataRequest&) = default;
  PublishPartitionDataRequest(PublishPartitionDataRequest&&) = default;
  PublishPartitionDataRequest& operator=(const PublishPartitionDataRequest&) =
      default;
  PublishPartitionDataRequest& operator=(PublishPartitionDataRequest&&) =
      default;
  virtual ~PublishPartitionDataRequest() = default;

  /**
   * @brief Gets the data to be published to the HERE platform.
   *
   * @return The data to be published.
   */
  inline std::shared_ptr<std::vector<unsigned char>> GetData() const {
    return data_;
  }

  /**
   * @brief Sets the data to be published to the HERE platform.
   *
   * @param data The data to be published.
   */
  inline PublishPartitionDataRequest& WithData(
      const std::shared_ptr<std::vector<unsigned char>>& data) {
    data_ = data;
    return *this;
  }

  /**
   * @brief Sets the data to be published to the HERE platform.
   *
   * @param data The rvalue reference to the data to be published.
   */
  inline PublishPartitionDataRequest& WithData(
      std::shared_ptr<std::vector<unsigned char>>&& data) {
    data_ = std::move(data);
    return *this;
  }

  /**
   * @brief Gets the layer ID of the catalog where you want to store the data.
   *
   * @return The layer ID of the catalog.
   */
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @brief Sets the layer ID of the catalog where you want to store the data.
   *
   * Make sure the layer is of the volative or versioned type.
   *
   * @param layer_id The layer ID of the catalog.
   */
  inline PublishPartitionDataRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @brief Sets the layer ID of the catalog where you want to store the data.
   *
   * Make sure the layer is of the volative or versioned type.
   *
   * @param layer_id The rvalue reference to the layer ID of the catalog.
   */
  inline PublishPartitionDataRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @brief Gets the ID of the partition to which you want to publish data.
   *
   * @return The partition ID.
   */
  inline const porting::optional<std::string>& GetPartitionId() const {
    return partition_id_;
  }

  /**
   * @brief Sets the ID of the partition to which you want to publish data.
   *
   * @param partition_id A key that specifies the partition to which
   * the content is related. If the layer partitioning scheme is set to
   * HERE tile, the partition key is the tile key. The maximum length of
   * the partition key is 500 characters.
   */
  inline PublishPartitionDataRequest& WithPartitionId(
      const std::string& partition_id) {
    partition_id_ = partition_id;
    return *this;
  }

  /**
   * @brief Sets the ID of the partition to which you want to publish data.
   *
   * @param partition_id A key that specifies the partition to which
   * the content is related. If the layer partitioning scheme is set to
   * HERE tile, the partition key is the tile key. The maximum length of
   * the partition key is 500 characters.
   */
  inline PublishPartitionDataRequest& WithPartitionId(
      std::string&& partition_id) {
    partition_id_ = std::move(partition_id);
    return *this;
  }

  /**
   * @brief Gets the billing tag to group billing records together.
   *
   * The billing tag is an optional free-form tag that is used for grouping
   * billing records together. If supplied, it must be 4–16 characters
   * long and contain only alphanumeric ASCII characters [A-Za-z0-9].
   *
   * @return The `BillingTag` string or `olp::porting::none` if the billing tag
   * is not set.
   */
  inline const porting::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param billing_tag The `BillingTag` string or `olp::porting::none`.
   */
  inline PublishPartitionDataRequest& WithBillingTag(
      const std::string& billing_tag) {
    billing_tag_ = billing_tag;
    return *this;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param billing_tag The rvalue reference to the `BillingTag` string or
   * `olp::porting::none`.
   */
  inline PublishPartitionDataRequest& WithBillingTag(
      std::string&& billing_tag) {
    billing_tag_ = std::move(billing_tag);
    return *this;
  }

  /**
   * @brief Gets the request checksum.
   *
   * It is an SHA-256 hash that you can provide for
   * validation against the calculated value on the request body hash.
   * It verifies the integrity of your request and prevents modification
   * by a third party. If not provided, it is created by the service.
   * The SHA-256 hash consists of 256 bits or 64 chars.
   *
   * @return The request checksum.
   */
  inline const porting::optional<std::string>& GetChecksum() const {
    return checksum_;
  }

  /**
   * @brief Sets the request checksum.
   *
   * @see `GetChecksum` for information on the checksum.
   *
   * @param checksum The request checksum.
   */
  inline PublishPartitionDataRequest& WithChecksum(
      const std::string& checksum) {
    checksum_ = checksum;
    return *this;
  }

  /**
   * @brief Sets the request checksum.
   *
   * @see `GetChecksum` for information on the checksum.
   *
   * @param checksum The rvalue reference to the request checksum.
   */
  inline PublishPartitionDataRequest& WithChecksum(std::string&& checksum) {
    checksum_ = std::move(checksum);
    return *this;
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> data_;

  std::string layer_id_;

  porting::optional<std::string> partition_id_;

  porting::optional<std::string> billing_tag_;

  porting::optional<std::string> checksum_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
