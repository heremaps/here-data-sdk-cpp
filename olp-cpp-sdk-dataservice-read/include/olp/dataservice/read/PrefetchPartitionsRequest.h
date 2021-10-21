/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/FetchOptions.h>
#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to prefetch a list of partitions for
 * the given catalog and layer.
 */
class DATASERVICE_READ_API PrefetchPartitionsRequest final {
 public:
  /// An alias for the vector of partitions IDs.
  using PartitionIds = std::vector<std::string>;

  /**
   * @brief Sets the list of partitions.
   *
   * When the list is empty, the GetPartitions method will download the whole
   * layer metadata. Additionally, a single request supports up to 100
   * partitions. If partitions list has more than 100, it will be split
   * internally to multiple requests.
   *
   * @param partition_ids The list of partitions to request.
   *
   * @return A reference to the updated `PrefetchPartitionsRequest` instance.
   */
  inline PrefetchPartitionsRequest& WithPartitionIds(
      PartitionIds partition_ids) {
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
   * @param billing_tag The `BillingTag` string or `boost::none`.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchPartitionsRequest& WithBillingTag(
      boost::optional<std::string> billing_tag) {
    billing_tag_ = std::move(billing_tag);
    return *this;
  }

  /**
   * @brief Sets the billing tag for the request.
   *
   * @see `GetBillingTag()` for information on usage and format.
   *
   * @param billing_tag The rvalue reference to the `BillingTag` string or
   * `boost::none`.
   *
   * @return A reference to the updated `PrefetchTilesRequest` instance.
   */
  inline PrefetchPartitionsRequest& WithBillingTag(std::string&& billing_tag) {
    billing_tag_ = std::move(billing_tag);
    return *this;
  }

  /**
   * @brief Gets the request priority.
   *
   * The default priority is `Priority::LOW`.
   *
   * @return The request priority.
   */
  inline uint32_t GetPriority() const { return priority_; }

  /**
   * @brief Sets the priority of the prefetch request.
   *
   * @param priority The priority of the request.
   *
   * @return A reference to the updated `PrefetchPartitionsRequest` instance.
   */
  inline PrefetchPartitionsRequest& WithPriority(uint32_t priority) {
    priority_ = priority;
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
  std::string CreateKey(const std::string& layer_id,
                        boost::optional<int64_t> version = boost::none) const {
    std::stringstream out;
    out << layer_id;
    if (version) {
      out << "@" << version.get();
    }
    out << "^" << partition_ids_.size();
    if (!partition_ids_.empty()) {
      out << "[" << partition_ids_.front() << ", ...]";
    }
    if (GetBillingTag()) {
      out << "$" << GetBillingTag().get();
    }
    out << "*" << priority_;
    return out.str();
  }

 private:
  PartitionIds partition_ids_;
  boost::optional<std::string> billing_tag_;
  uint32_t priority_{thread::LOW};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
