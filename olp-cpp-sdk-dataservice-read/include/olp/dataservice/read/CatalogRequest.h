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

#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/FetchOptions.h>
#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request catalog configuration.
 */
class DATASERVICE_READ_API CatalogRequest final {
 public:
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
   * @param tag The `BillingTag` string or `boost::none`.
   * @return A reference to the updated `CatalogRequest` instance.
   */
  inline CatalogRequest& WithBillingTag(boost::optional<std::string> tag) {
    billing_tag_ = std::move(tag);
    return *this;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param tag The rvalue reference to the `BillingTag` string or
   * `boost::none`.
   * @return A reference to the updated `CatalogRequest` instance.
   */
  inline CatalogRequest& WithBillingTag(std::string&& tag) {
    billing_tag_ = std::move(tag);
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
   * @return A reference to the updated `CatalogVersionRequest` instance.
   */
  inline CatalogRequest& WithFetchOption(FetchOptions fetch_option) {
    fetch_option_ = fetch_option;
    return *this;
  }

  /**
   * @brief Creates a readable format of the request.
   *
   * @return A string representation of the request.
   */
  inline std::string CreateKey() const {
    std::stringstream out;
    if (GetBillingTag()) {
      out << "$" << GetBillingTag().get();
    }
    out << "^" << GetFetchOption();
    return out.str();
  }

 private:
  boost::optional<std::string> billing_tag_;
  FetchOptions fetch_option_{OnlineIfNotFound};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
