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
 * @brief PublishDataRequest used to publish data into an OLP Stream Layer.
 */
class DATASERVICE_WRITE_API PublishDataRequest {
 public:
  PublishDataRequest() = default;

  /**
   * @return data previously set.
   */
  inline std::shared_ptr<std::vector<unsigned char>> GetData() const {
    return data_;
  }

  /**
   * @param data Content to be uploaded to OLP.
   * @note Required.
   */
  inline PublishDataRequest& WithData(
      const std::shared_ptr<std::vector<unsigned char>>& data) {
    data_ = data;
    return *this;
  }

  /**
   * @param data Content to be uploaded to OLP.
   * @note Required.
   */
  inline PublishDataRequest& WithData(
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
   * The layer type must be Stream.
   * @note Required.
   */
  inline PublishDataRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @param layer_id Layer ID of the catalog where you want to store the data.
   * The layer type must be Stream.
   * @note Required.
   */
  inline PublishDataRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @return Trace ID previously set.
   */
  inline const boost::optional<std::string>& GetTraceId() const {
    return trace_id_;
  }

  /**
   * @param trace_id A unique message ID, such as a UUID. This can be included
   * in the request if you want to use an ID that you define. If you do not
   * include an ID, one will be generated during ingestion and included in the
   * response. You can use this ID to track your request and identify the
   * message in the catalog.
   * @note Optional.
   */
  inline PublishDataRequest& WithTraceId(const std::string& trace_id) {
    trace_id_ = trace_id;
    return *this;
  }

  /**
   * @param trace_id A unique message ID, such as a UUID. This can be included
   * in the request if you want to use an ID that you define. If you do not
   * include an ID, one will be generated during ingestion and included in the
   * response. You can use this ID to track your request and identify the
   * message in the catalog.
   * @note Optional.
   */
  inline PublishDataRequest& WithTraceId(std::string&& trace_id) {
    trace_id_ = std::move(trace_id);
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
  inline PublishDataRequest& WithBillingTag(const std::string& billing_tag) {
    billing_tag_ = billing_tag;
    return *this;
  }

  /**
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters [A-Za-z0-9].
   * @note Optional.
   */
  inline PublishDataRequest& WithBillingTag(std::string&& billing_tag) {
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
  inline PublishDataRequest& WithChecksum(const std::string& checksum) {
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
  inline PublishDataRequest& WithChecksum(std::string&& checksum) {
    checksum_ = std::move(checksum);
    return *this;
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> data_;

  std::string layer_id_;

  boost::optional<std::string> trace_id_;

  boost::optional<std::string> billing_tag_;

  boost::optional<std::string> checksum_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
