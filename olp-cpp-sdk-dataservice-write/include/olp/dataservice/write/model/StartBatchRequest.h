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

#include <string>
#include <utility>
#include <vector>

#include <olp/core/porting/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/VersionDependency.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

/// Starts a versioned batch operation.
class DATASERVICE_WRITE_API StartBatchRequest {
 public:
  StartBatchRequest() = default;
  StartBatchRequest(const StartBatchRequest&) = default;
  StartBatchRequest(StartBatchRequest&&) = default;
  StartBatchRequest& operator=(const StartBatchRequest&) = default;
  StartBatchRequest& operator=(StartBatchRequest&&) = default;
  virtual ~StartBatchRequest() = default;

  /**
   * @brief Sets the layers used in the batch operation.
   *
   * @param layers The layer IDs.
   */
  StartBatchRequest& WithLayers(std::vector<std::string> layers) {
    layers_ = std::move(layers);
    return *this;
  }

  /**
   * @brief Gets the layers used in the batch operation.
   *
   * @return The layer IDs.
   */
  const porting::optional<std::vector<std::string>>& GetLayers() const {
    return layers_;
  }

  /**
   * @brief Sets the version dependencies used in the batch operation.
   *
   * @param versionDependencies The version dependencies.
   */
  StartBatchRequest& WithVersionDependencies(
      std::vector<VersionDependency> versionDependencies) {
    versionDependencies_ = std::move(versionDependencies);
    return *this;
  }

  /**
   * @brief Gets the version dependencies used in the batch operation.
   *
   * @return The version dependencies.
   */
  const porting::optional<std::vector<VersionDependency>>&
  GetVersionDependencies() const {
    return versionDependencies_;
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
   * @param billing_tag The `BillingTag` string or `olp::porting::none`.
   */
  StartBatchRequest& WithBillingTag(std::string billing_tag) {
    billing_tag_ = std::move(billing_tag);
    return *this;
  }

 private:
  porting::optional<std::vector<std::string>> layers_;
  porting::optional<std::vector<VersionDependency>> versionDependencies_;
  porting::optional<std::string> billing_tag_;
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
