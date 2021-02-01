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

#include <sstream>
#include <string>
#include <utility>

#include <boost/optional.hpp>

#include "DataServiceReadApi.h"
#include "FetchOptions.h"

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request catalog configuration.
 */
class DATASERVICE_READ_API CatalogVersionRequest final {
 public:
  /**
   * @brief Gets the catalog start version (exclusive) for the request.
   *
   * Mandatory for versioned layers.
   * By convention -1 indicates the initial version before the first
   * publication. After the first publication, the catalog version is 0.
   *
   * @return The catalog start version.
   */
  inline int64_t GetStartVersion() const { return start_version_; }

  /**
   * @brief Sets the catalog start version.
   *
   * @see `GetStartVersion()` for information on usage.
   *
   * @param startVersion The catalog start version.
   *
   * @return A reference to the updated `CatalogVersionRequest` instance.
   */
  inline CatalogVersionRequest& WithStartVersion(int64_t startVersion) {
    start_version_ = startVersion;
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
   * @param billingTag The `BillingTag` string or `boost::none`.
   *
   * @return A reference to the updated `CatalogVersionRequest` instance.
   */
  inline CatalogVersionRequest& WithBillingTag(
      boost::optional<std::string> billingTag) {
    billing_tag_ = std::move(billingTag);
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
   * @return A reference to the updated `CatalogVersionRequest` instance.
   */
  inline CatalogVersionRequest& WithBillingTag(std::string&& billingTag) {
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
   * @param fetchoption The `FetchOption` enum.
   *
   * @return A reference to the updated `CatalogVersionRequest` instance.
   */
  inline CatalogVersionRequest& WithFetchOption(FetchOptions fetchoption) {
    fetch_option_ = fetchoption;
    return *this;
  }

  /**
   * @brief Creates a readable format of the request.
   *
   * @return A string representation of the request.
   */
  inline std::string CreateKey() const {
    std::stringstream out;

    out << "@" << GetStartVersion();

    if (GetBillingTag()) {
      out << "$" << GetBillingTag().get();
    }

    out << "^" << GetFetchOption();

    return out.str();
  }

 private:
  int64_t start_version_{0};
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_{OnlineIfNotFound};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
