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

#include <olp/core/porting/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/// Publishes data to a stream layer.
class DATASERVICE_WRITE_API PublishDataRequest {
 public:
  PublishDataRequest() = default;
  PublishDataRequest(const PublishDataRequest&) = default;
  PublishDataRequest(PublishDataRequest&&) = default;
  PublishDataRequest& operator=(const PublishDataRequest&) = default;
  PublishDataRequest& operator=(PublishDataRequest&&) = default;
  virtual ~PublishDataRequest() = default;

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
  inline PublishDataRequest& WithData(
      const std::shared_ptr<std::vector<unsigned char>>& data) {
    data_ = data;
    return *this;
  }

  /**
   * @brief Sets the data to be published to the HERE platform.
   *
   * @param data The rvalue reference to the data to be published.
   */
  inline PublishDataRequest& WithData(
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
   * Make sure the layer is of the stream type.
   *
   * @param layer_id The layer ID of the catalog.
   */
  inline PublishDataRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @brief Sets the layer ID of the catalog where you want to store the data.
   *
   * Make sure the layer is of the stream type.
   *
   * @param layer_id The rvalue reference to the layer ID of the catalog.
   */
  inline PublishDataRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @brief Gets the trace ID of the request.
   *
   * It is a unique message ID, such as a UUID.
   * You can use this ID to track your request and identify the
   * message in the catalog.
   *
   * @return The trace ID of the request.
   */
  inline const porting::optional<std::string>& GetTraceId() const {
    return trace_id_;
  }

  /**
   * @brief Sets the trace ID of the request.
   *
   * @param trace_id A unique message ID, such as a UUID. If you want to
   * define your ID, include it in the request. If you do not
   * include an ID, it is generated during ingestion and included in
   * the response. You can use this ID to track your request and identify
   * the message in the catalog.
   */
  inline PublishDataRequest& WithTraceId(const std::string& trace_id) {
    trace_id_ = trace_id;
    return *this;
  }

  /**
   * @brief Sets the trace ID of the request.
   *
   * @param trace_id The rvalue reference to the unique message ID,
   * such as a UUID. If you want to define your ID, include it in the request.
   * If you do not include an ID, it is generated during ingestion and included
   * in the response. You can use this ID to track your request and identify
   * the message in the catalog.
   */
  inline PublishDataRequest& WithTraceId(std::string&& trace_id) {
    trace_id_ = std::move(trace_id);
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
  inline PublishDataRequest& WithBillingTag(const std::string& billing_tag) {
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
  inline PublishDataRequest& WithBillingTag(std::string&& billing_tag) {
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
  inline PublishDataRequest& WithChecksum(const std::string& checksum) {
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
  inline PublishDataRequest& WithChecksum(std::string&& checksum) {
    checksum_ = std::move(checksum);
    return *this;
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> data_;

  std::string layer_id_;

  porting::optional<std::string> trace_id_;

  porting::optional<std::string> billing_tag_;

  porting::optional<std::string> checksum_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
