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
 * @brief Encapsulates the fields required to request a list of partitions for
 * a given catalog and layer.
 */
class DATASERVICE_READ_API PartitionsRequest final {
 public:
  /**
   * @brief Sets the catalog metadata version for the request of the requested
   * partitions.
   *
   * @param catalogMetadataVersion The catalog metadata version of the requested
   * partitions. If the version is not specified, the latest version is used.
   * @return A reference to the updated `PartitionsRequest` instance.
   */
  inline PartitionsRequest& WithVersion(
      boost::optional<int64_t> catalogMetadataVersion) {
    catalog_metadata_version_ = catalogMetadataVersion;
    return *this;
  }

  /**
   * @brief Gets the catalog metadata version of the requested partitions.
   *
   * @return The catalog metadata version.
   */
  inline const boost::optional<std::int64_t>& GetVersion() const {
    return catalog_metadata_version_;
  }

  /**
   * @brief Gets a billing tag to group billing records together.
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
   * @see `GetBillingTag()` for usage and format.
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
   * @see `GetBillingTag()` for usage and format.
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
   * @brief Gets a fetch option that controls how requests are handled.
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
   * @see `GetFetchOption()` for usage and format.
   *
   * @param fetchoption The `FetchOption` enum.
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PartitionsRequest& WithFetchOption(FetchOptions fetchoption) {
    fetch_option_ = fetchoption;
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
  std::string layer_id_;
  boost::optional<int64_t> catalog_metadata_version_;
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_ = OnlineIfNotFound;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
