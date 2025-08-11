/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/porting/optional.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/FetchOptions.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request a list of partitions for
 * the given catalog and layer.
 */
class DATASERVICE_READ_API PartitionsRequest final {
 public:
  /// An alias for the vector of partitions IDs.
  using PartitionIds = std::vector<std::string>;

  /// Additional field to request partition data size, see GetPartitions
  static constexpr const char* kDataSize = "dataSize";

  /// Additional field to request partition checksum, see GetPartitions
  static constexpr const char* kChecksum = "checksum";

  /// Additional field to request partition compressed data size, see
  /// GetPartitions
  static constexpr const char* kCompressedDataSize = "compressedDataSize";

  /// Additional field to request partition crc, see GetPartitions
  static constexpr const char* kCrc = "crc";

  /// An alias for the set of additional fields.
  using AdditionalFields = std::vector<std::string>;

  /**
   * @brief Sets the list of partitions.
   *
   * When the list is empty, the GetPartitions method will download the whole
   * layer metadata. Additionally, a single request supports up to 100
   * partitions.
   *
   * @param partition_ids The list of partitions to request.
   *
   * @return A reference to the updated `PartitionsRequest` instance.
   */
  PartitionsRequest& WithPartitionIds(PartitionIds partition_ids) {
    partition_ids_ = std::move(partition_ids);
    return *this;
  }

  /**
   * @brief Gets the list of the partitions.
   *
   * @return The vector of strings that represent partitions.
   */
  const PartitionIds& GetPartitionIds() const { return partition_ids_; }

  /**
   * @brief Sets the list of additional fields.
   *
   * When specified, the result metadata will include the additional information
   * requested. The supported fields are:
   *  - dataSize
   *  - checksum
   *  - compressedDataSize
   *  - crc
   *
   * @param additional_fields The list of additional fields.
   *
   * @return A reference to the updated `PartitionsRequest` instance.
   */
  PartitionsRequest& WithAdditionalFields(AdditionalFields additional_fields) {
    additional_fields_ = std::move(additional_fields);
    return *this;
  }

  /**
   * @brief Gets the list of additional fields.
   *
   * @return The set of additional fields.
   */
  const AdditionalFields& GetAdditionalFields() const {
    return additional_fields_;
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
  const porting::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param billingTag The `BillingTag` string or `olp::porting::none`.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  template <class T = porting::optional<std::string>>
  PartitionsRequest& WithBillingTag(T&& billingTag) {
    billing_tag_ = std::forward<T>(billingTag);
    return *this;
  }

  /**
   * @brief Gets the fetch option that controls how requests are handled.
   *
   * The default option is `OnlineIfNotFound` that queries the network if
   * the requested resource is not in the cache.
   *
   * @return The fetch option.
   */
  FetchOptions GetFetchOption() const { return fetch_option_; }

  /**
   * @brief Sets the fetch option that you can use to set the source from
   * which data should be fetched.
   *
   * @see `GetFetchOption()` for information on usage and format.
   *
   * @param fetch_option The `FetchOption` enum.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  PartitionsRequest& WithFetchOption(FetchOptions fetch_option) {
    fetch_option_ = fetch_option;
    return *this;
  }

  /**
   * @brief Creates a readable format for the request.
   *
   * @param layer_id The ID of the layer that is used for the request.
   * @param version The catalog version.
   *
   * @return A string representation of the request.
   */
  std::string CreateKey(
      const std::string& layer_id,
      porting::optional<int64_t> version = porting::none) const {
    std::stringstream out;
    out << layer_id;
    if (version) {
      out << "@" << *version;
    }
    if (GetBillingTag()) {
      out << "$" << *GetBillingTag();
    }
    out << "^" << GetFetchOption();
    return out.str();
  }

 private:
  PartitionIds partition_ids_;
  AdditionalFields additional_fields_;
  porting::optional<std::string> billing_tag_;
  FetchOptions fetch_option_{OnlineIfNotFound};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
