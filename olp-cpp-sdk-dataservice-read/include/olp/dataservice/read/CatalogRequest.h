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

#include "FetchOptions.h"

#include <string>

#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief The CatalogRequest class encapsulates the fields required to request
 * the Catalog configuration.
 */
class CatalogRequest final {
 public:
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
   * @return a reference to the updated CatalogRequest
   */
  inline CatalogRequest& WithBillingTag(
      boost::optional<std::string> billingTag) {
    billing_tag_ = billingTag;
    return *this;
  }

  /**
   * @brief WithBillingTag sets the billing tag. See ::GetBillingTag() for usage
   * and format.
   * @param billingTag a string or boost::none
   * @return a reference to the updated CatalogRequest
   */
  inline CatalogRequest& WithBillingTag(std::string&& billingTag) {
    billing_tag_ = std::move(billingTag);
    return *this;
  }

  /**
   * @brief FetchOption will control how requests are handled. Default option
   * is OnlineIfNotFound, which will query the network only if the requested
   * resource is not in the cache.
   * @return the fetchOption
   */
  inline FetchOptions GetFetchOption() const {
    return fetch_option_;
  }

  /**
   * @brief WithFetchOption sets the fetch option. See ::GetFetchOption() for
   * usage and format.
   * @param fetchoption enums
   * @return a reference to the updated CatalogRequest
   */
  inline CatalogRequest& WithFetchOption(FetchOptions fetchoption) {
    fetch_option_ = fetchoption;
    return *this;
  }
 private:
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_ = OnlineIfNotFound;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
