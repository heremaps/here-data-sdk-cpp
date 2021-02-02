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

#include <string>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/VersionDependency.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

/**
 * @brief StartBatchRequest is use to start a versioned batch operation.
 */
class DATASERVICE_WRITE_API StartBatchRequest {
 public:
  StartBatchRequest() = default;

  /**
   * @brief set the layers used in this batch operation
   * @param layers layer id's to be used
   * @return reference to this object
   */
  inline StartBatchRequest& WithLayers(std::vector<std::string> layers) {
    layers_ = std::move(layers);
    return *this;
  }

  /**
   * @brief get the layers used in this batch operation
   * @return collection of layers in boost::optional
   */
  inline const boost::optional<std::vector<std::string>>& GetLayers() const {
    return layers_;
  }

  /**
   * @brief set the version dependencies used in this batch operation
   * @param versionDependencies collection of VersionDependencies
   * @return reference to this object
   */
  inline StartBatchRequest& WithVersionDependencies(
      std::vector<VersionDependency> versionDependencies) {
    versionDependencies_ = std::move(versionDependencies);
    return *this;
  }

  /**
   * @brief gets the VersionDependencies of this batch operation
   * @return boost::optional holding the collection of VersionDependencies
   */
  inline const boost::optional<std::vector<VersionDependency>>&
  GetVersionDependencies() const {
    return versionDependencies_;
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
  inline StartBatchRequest& WithBillingTag(std::string billing_tag) {
    billing_tag_ = std::move(billing_tag);
    return *this;
  }

 private:
  boost::optional<std::vector<std::string>> layers_;
  boost::optional<std::vector<VersionDependency>> versionDependencies_;
  boost::optional<std::string> billing_tag_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
