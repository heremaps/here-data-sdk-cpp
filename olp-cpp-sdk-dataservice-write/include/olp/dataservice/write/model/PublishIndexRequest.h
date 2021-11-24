/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <boost/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/Index.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/// Publishes data to an index layer.
class DATASERVICE_WRITE_API PublishIndexRequest {
 public:
  PublishIndexRequest() = default;

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
  inline PublishIndexRequest& WithData(
      const std::shared_ptr<std::vector<unsigned char>>& data) {
    data_ = data;
    return *this;
  }

  /**
   * @brief Sets the data to be published to the HERE platform.
   *
   * @param data The rvalue reference to the data to be published.
   */
  inline PublishIndexRequest& WithData(
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
   * Make sure the layer is of the index type.
   *
   * @param layer_id The layer ID of the catalog.
   */
  inline PublishIndexRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @brief Sets the layer ID of the catalog where you want to store the data.
   *
   * Make sure the layer is of the index type.
   *
   * @param layer_id The rvalue reference to the layer ID of the catalog.
   */
  inline PublishIndexRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @brief Gets the billing tag to group billing records together.
   *
   * The billing tag is an optional free-form tag that is used for grouping
   * billing records together. If supplied, it must be 4–16 characters
   * long and contain only alphanumeric ASCII characters [A-Za-z0-9].
   *
   * @return The `BillingTag` string or `boost::none` if the billing tag is not
   * set.
   */
  inline const boost::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param billing_tag The rvalue reference to the `BillingTag` string or
   * `boost::none`.
   */
  inline PublishIndexRequest& WithBillingTag(const std::string& billing_tag) {
    billing_tag_ = billing_tag;
    return *this;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param billing_tag The rvalue reference to the `BillingTag` string or
   * `boost::none`.
   */
  inline PublishIndexRequest& WithBillingTag(std::string&& billing_tag) {
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
  inline const boost::optional<std::string>& GetChecksum() const {
    return checksum_;
  }

  /**
   * @brief Sets the request checksum.
   *
   * @see `GetChecksum` for information on the checksum.
   *
   * @param checksum The request checksum.
   */
  inline PublishIndexRequest& WithChecksum(const std::string& checksum) {
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
  inline PublishIndexRequest& WithChecksum(std::string&& checksum) {
    checksum_ = std::move(checksum);
    return *this;
  }

  /**
   * @brief Gets the `Index` object that contains the index information
   * to be published to the index layer.
   *
   * It consists of the following fields: metadata, ID, types of index fields,
   * and size.
   *
   * @see `Index` for more information.
   *
   * @return The `Index` object. 
   */
  inline const Index& GetIndex() const { return index_; }

  /**
   * @brief Sets the `Index` object.
   *
   * @see `Index` for more information.
   *
   * @param index The `Index` object.
   */
  inline PublishIndexRequest& WithIndex(const Index& index) {
    index_ = index;
    return *this;
  }

  /**
   * @brief Sets the `Index` object.
   *
   * @see `Index` for more information.
   *
   * @param index The rvalue reference to the `Index` object.
   */
  inline PublishIndexRequest& WithIndex(Index&& index) {
    index_ = std::move(index);
    return *this;
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> data_;

  std::string layer_id_;

  boost::optional<std::string> billing_tag_;

  boost::optional<std::string> checksum_;

  Index index_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
