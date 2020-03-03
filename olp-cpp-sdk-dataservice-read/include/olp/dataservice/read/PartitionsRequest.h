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
#include <vector>

#include <olp/core/porting/deprecated.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/FetchOptions.h>
#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request a list of partitions for
 * the given catalog and layer.
 */
class DATASERVICE_READ_API PartitionsRequest final {
 public:
  /// The alias type of the vector of partitions ids
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

  /// The alias type of the set of additional fields
  using AdditionalFields = std::vector<std::string>;

  /**
   * @brief Sets the list of partitions.
   *
   * When the list is empty, the GetPartitions method will download the whole
   * layer metadata. Additionally, a single request supports up to 100
   * partitions.
   *
   * @param partitions The list of partitions to request.
   *
   * @return A reference to the updated `PartitionsRequest` instance.
   */
  inline PartitionsRequest& WithPartitionIds(PartitionIds partition_ids) {
    partition_ids_ = std::move(partition_ids);
    return *this;
  }

  /**
   * @brief Gets the list of the partitions.
   *
   * @return The vector of strings that represent partitions.
   */
  inline const PartitionIds& GetPartitionIds() const { return partition_ids_; }

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
   * @param fields The list of additional fields.
   *
   * @return A reference to the updated `PartitionsRequest` instance.
   */
  inline PartitionsRequest& WithAdditionalFields(
      AdditionalFields additional_fields) {
    additional_fields_ = std::move(additional_fields);
    return *this;
  }

  /**
   * @brief Gets the list of additional fields.
   *
   * @return The set of additional fields.
   */
  inline const AdditionalFields& GetAdditionalFields() const {
    return additional_fields_;
  }

  /**
   * @brief Sets the catalog metadata version.
   *
   * @param catalog_version The catalog metadata version of the requested
   * partitions. If the version is not specified, the latest version is
   * retrieved.
   *
   * @return A reference to the updated `PartitionsRequest` instance.
   *
   * @deprecated The version is now a part of the VersionedLayerClient
   * constructor.
   */
  OLP_SDK_DEPRECATED("Deprecated, to be removed in 06.2020")
  inline PartitionsRequest& WithVersion(
      boost::optional<int64_t> catalog_version) {
    catalog_version_ = std::move(catalog_version);
    return *this;
  }

  /**
   * @brief Gets the catalog metadata version of the requested partitions.
   *
   * @return The catalog metadata version.
   *
   * @deprecated The version is now a part of the VersionedLayerClient
   * constructor.
   */
  OLP_SDK_DEPRECATED("Deprecated, to be removed in 06.2020")
  inline const boost::optional<std::int64_t>& GetVersion() const {
    return catalog_version_;
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
   * @param billingTag The `BillingTag` string or `boost::none`.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PartitionsRequest& WithBillingTag(
      boost::optional<std::string> billingTag) {
    billing_tag_ = billingTag;
    return *this;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param billingTag The rvalue reference to the `BillingTag` string or
   * `boost::none`.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PartitionsRequest& WithBillingTag(std::string&& billingTag) {
    billing_tag_ = std::move(billingTag);
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
  inline FetchOptions GetFetchOption() const { return fetch_option_; }

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
  inline PartitionsRequest& WithFetchOption(FetchOptions fetch_option) {
    fetch_option_ = fetch_option;
    return *this;
  }

  /**
   * @brief Creates a readable format for the request.
   *
   * @param layer_id The ID of the layer that is used for the request.
   *
   * @return A string representation of the request.
   */
  std::string CreateKey(const std::string& layer_id) const {
    std::stringstream out;
    out << layer_id;
    if (GetVersion()) {
      out << "@" << GetVersion().get();
    }
    if (GetBillingTag()) {
      out << "$" << GetBillingTag().get();
    }
    out << "^" << GetFetchOption();
    return out.str();
  }

 private:
  PartitionIds partition_ids_;
  AdditionalFields additional_fields_;
  boost::optional<int64_t> catalog_version_;
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_{OnlineIfNotFound};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
