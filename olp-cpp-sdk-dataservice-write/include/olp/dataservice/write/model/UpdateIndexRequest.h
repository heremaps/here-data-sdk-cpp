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
/**
 * @brief UpdateIndexRequest used to update index to
 * a HERE platform index layer.
 */
class DATASERVICE_WRITE_API UpdateIndexRequest {
 public:
  UpdateIndexRequest() = default;

  /**
   * @return Layer ID previously set.
   */
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @param layer_id Layer ID of the catalog where you want to store the data.
   * The layer type must be Index.
   * @note Required.
   */
  inline UpdateIndexRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @param layer_id Layer ID of the catalog where you want to store the data.
   * The layer type must be Index.
   * @note Required.
   */
  inline UpdateIndexRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
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
  inline UpdateIndexRequest& WithBillingTag(const std::string& billing_tag) {
    billing_tag_ = billing_tag;
    return *this;
  }

  /**
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @note Optional.
   */
  inline UpdateIndexRequest& WithBillingTag(std::string&& billing_tag) {
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
  inline UpdateIndexRequest& WithChecksum(const std::string& checksum) {
    checksum_ = checksum;
    return *this;
  }

  /**
   * @param checksum A SHA-256 hash you can provide for
   * validation against the calculated value on the request body hash. This
   * verifies the integrity of your request and prevents modification by a third
   * party.It will be created by the service if not provided. A SHA-256 hash
   * consists of 256 bits or 64 chars.
   * @note Required.
   */
  inline UpdateIndexRequest& WithChecksum(std::string&& checksum) {
    checksum_ = std::move(checksum);
    return *this;
  }

  /**
   * @return IndexAdditions previously set.
   */
  inline const std::vector<Index>& GetIndexAdditions() const {
    return indexAdditions_;
  }

  /**
   * @param indexAdditions Contains indexes that need to be newly added to an
   * index layer.
   * @note Required.
   */
  inline UpdateIndexRequest& WithIndexAdditions(
      const std::vector<Index>& indexAdditions) {
    indexAdditions_ = indexAdditions;
    return *this;
  }

  /**
   * @param indexAdditions Contains indexes that need to be newly added to an
   * index layer.
   * @note Required.
   */
  inline UpdateIndexRequest& WithIndexAdditions(
      std::vector<Index>&& indexAdditions) {
    indexAdditions_ = std::move(indexAdditions);
    return *this;
  }

  /**
   * @return IndexRemovals previously set.
   */
  inline const std::vector<std::string>& GetIndexRemovals() const {
    return indexRemovals_;
  }

  /**
   * @param indexRemovals Contains dataHandles which associate indexes need to
   * be removed from an index layer.
   * @note Required.
   */
  inline UpdateIndexRequest& WithIndexRemovals(
      const std::vector<std::string>& indexRemovals) {
    indexRemovals_ = indexRemovals;
    return *this;
  }

  /**
   * @param indexRemovals Contains dataHandles which associate indexes need to
   * be removed from an index layer.
   * @note Required.
   */
  inline UpdateIndexRequest& WithIndexRemovals(
      std::vector<std::string>&& indexRemovals) {
    indexRemovals_ = std::move(indexRemovals);
    return *this;
  }

 private:
  std::string layer_id_;

  boost::optional<std::string> billing_tag_;

  boost::optional<std::string> checksum_;

  std::vector<Index> indexAdditions_;

  std::vector<std::string> indexRemovals_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
