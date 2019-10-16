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
 * @brief The PartitionsRequest class encapsulates the fields required to
 * request the list of partitions for a given catalog and layer.
 */
class DATASERVICE_READ_API PartitionsRequest final {
 public:
  /**
   * @brief GetLayerId gets the request's layer id.
   * @return the layer id.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @brief WithLayerId sets the layer id of the request.
   * @param layerId the layer the requested partitions belong to.
   * @return a reference to the updated PartitionsRequest.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  inline PartitionsRequest& WithLayerId(const std::string& layerId) {
    layer_id_ = layerId;
    return *this;
  }

  /**
   * @brief WithLayerId sets the layer id of the request.
   * @param layerId the layer the requested partitions belong to.
   * @return a reference to the updated PartitionsRequest.
   * @deprecated Will be removed in 1.0
   */
  OLP_SDK_DEPRECATED("Will be removed in 1.0")
  inline PartitionsRequest& WithLayerId(std::string&& layerId) {
    layer_id_ = std::move(layerId);
    return *this;
  }

  /**
   * @brief WithVersion sets the catalog metadata version of the request.
   * @param catalogMetadataVersion The catalog metadta version of the requested
   * partions. If no version is specified, the latest will be retrieved.
   * @return a reference to the updated PartitionsRequest.
   */
  inline PartitionsRequest& WithVersion(
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
   * @return a reference to the updated PartitionsRequest
   */
  inline PartitionsRequest& WithBillingTag(
      boost::optional<std::string> billingTag) {
    billing_tag_ = billingTag;
    return *this;
  }

  /**
   * @brief WithBillingTag sets the billing tag. See ::GetBillingTag() for usage
   * and format.
   * @param billingTag a string or boost::none
   * @return a reference to the updated PartitionsRequest
   */
  inline PartitionsRequest& WithBillingTag(std::string&& billingTag) {
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
   * @return a reference to the updated PartitionsRequest
   */
  inline PartitionsRequest& WithFetchOption(FetchOptions fetchoption) {
    fetch_option_ = fetchoption;
    return *this;
  }

  /**
   * @brief Creates readable format for the request.
   * @param none
   * @return string representation of the request
   */
  std::string CreateKey() const {
    std::stringstream out;
    out << GetLayerId();

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
  std::string layer_id_;
  boost::optional<int64_t> catalog_metadata_version_;
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_ = OnlineIfNotFound;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
