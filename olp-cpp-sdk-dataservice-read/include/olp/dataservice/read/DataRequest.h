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

#include <sstream>
#include <string>

#include <olp/core/porting/deprecated.h>
#include <boost/optional.hpp>
#include "DataServiceReadApi.h"
#include "FetchOptions.h"

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief The DataRequest class encapsulates the fields required to
 * request the data for a given catalog and layer and partition.
 *
 * Either Partition Id or Data Handle must be provided. If both Partition Id and
 * Data Handle are set in the request, the Request fails with an
 * ErrorCode::PreconditionFailed Error.
 */
class DATASERVICE_READ_API DataRequest final {
 public:
  /**
   * @brief GetPartitionId gets the request's Partition Id.
   * @return the partition id.
   */
  inline const boost::optional<std::string>& GetPartitionId() const {
    return partition_id_;
  }

  /**
   * @brief WithPartitionId sets the request's Partition Id. If the partition
   * cannot be found in the layer, the callback will come back with an empty
   * response (null for data and error)
   * @param partitionId the Partition Id.
   * @return a reference to the updated DataRequest.
   */
  inline DataRequest& WithPartitionId(
      boost::optional<std::string> partitionId) {
    partition_id_ = partitionId;
    return *this;
  }

  /**
   * @brief WithPartitionId sets the request's Partition Id. If the partition
   * cannot be found in the layer, the callback will come back with an empty
   * response (null for data and error).
   * @param partitionId the Partition Id.
   * @return a reference to the updated DataRequest.
   */
  inline DataRequest& WithPartitionId(std::string&& partitionId) {
    partition_id_ = std::move(partitionId);
    return *this;
  }

  /**
   * @brief WithVersion sets the catalog metadata version of the request.
   * @param catalogMetadataVersion The catalog metadta version of the requested
   * partions. If no version is specified, the latest will be retrieved.
   * @return a reference to the updated PartitionsRequest.
   */
  inline DataRequest& WithVersion(
      boost::optional<int64_t> catalogMetadataVersion) {
    catalog_metadata_version_ = catalogMetadataVersion;
    return *this;
  }

  /**
   * @brief Get the catalog metadata version requested for the partitions.
   * @return the billing tag, or boost::none if not set.
   */
  inline const boost::optional<std::int64_t>& GetVersion() const {
    return catalog_metadata_version_;
  }

  /**
   * @brief GetDataHandle gets the request's Data Handle.
   * @return the data handle.
   */
  inline const boost::optional<std::string>& GetDataHandle() const {
    return data_handle_;
  }

  /**
   * @brief WithDataHandle sets the request's Data Handle. If the data handle
   * cannot be found in the layer, the callback will come back with an empty
   * response (null for data and error).
   * @param dataHandle the Data Handle.
   * @return a reference to the updated DataRequest.
   */
  inline DataRequest& WithDataHandle(boost::optional<std::string> dataHandle) {
    data_handle_ = dataHandle;
    return *this;
  }

  /**
   * @brief WithDataHandle sets the request's Data Handle. If the data handle
   * cannot be found in the layer, the callback will come back with an empty
   * response (null for data and error).
   * @param dataHandle the Data Handle.
   * @return a reference to the updated DataRequest.
   */
  inline DataRequest& WithDataHandle(std::string&& dataHandle) {
    data_handle_ = std::move(dataHandle);
    return *this;
  }

  /**
   * @brief BillingTag is an optional free-form tag which is used for
   * grouping billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @return the billing tag, or boost::none if not set.
   */
  inline const boost::optional<std::string>& GetBillingTag() const {
    return billing_tag_;
  }

  /**
   * @brief WithBillingTag sets the billing tag. See ::GetBillingTag() for usage
   * and format.
   * @param billingTag a string or boost::none
   * @return a reference to the updated DataRequest
   */
  inline DataRequest& WithBillingTag(
      const boost::optional<std::string>& billingTag) {
    billing_tag_ = billingTag;
    return *this;
  }

  /**
   * @brief WithBillingTag sets the billing tag. See ::GetBillingTag() for usage
   * and format.
   * @param billingTag a string or boost::none
   * @return a reference to the updated DataRequest
   */
  inline DataRequest& WithBillingTag(std::string&& billingTag) {
    billing_tag_ = std::move(billingTag);
    return *this;
  }

  /**
   * @brief FetchOption will control how requests are handled. Default option
   * is OnlineIfNotFound, which will query the network only if the requested
   * resource is not in the cache.
   * @return the fetchOption
   */
  inline FetchOptions GetFetchOption() const { return fetch_option_; }

  /**
   * @brief WithFetchOption sets the fetch option. See ::GetFetchOption() for
   * usage and format.
   * @param fetchoption enums
   * @return a reference to the updated DataRequest
   */
  inline DataRequest& WithFetchOption(FetchOptions fetchoption) {
    fetch_option_ = fetchoption;
    return *this;
  }

  /**
   * @brief Creates readable format for the request.
   * @param layer_id Layer ID request is used for.
   * @return string representation of the request.
   */
  inline std::string CreateKey(const std::string& layer_id) const {
    std::stringstream out;
    out << layer_id;

    out << "[";

    if (GetPartitionId()) {
      out << GetPartitionId().get();
    } else if (GetDataHandle()) {
      out << GetDataHandle().get();
    }
    out << "]";

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
  boost::optional<std::string> partition_id_;
  boost::optional<int64_t> catalog_metadata_version_;
  boost::optional<std::string> data_handle_;
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_ = OnlineIfNotFound;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
