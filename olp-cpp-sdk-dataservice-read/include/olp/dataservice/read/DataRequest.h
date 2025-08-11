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
#include <utility>

#include <olp/core/porting/optional.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/FetchOptions.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request data for the given
 * catalog, layer, and partition.
 *
 * You should specify either a partition ID or a data handle. If both the
 * partition ID and data handle are set in the request, the request fails with
 * the following error: `ErrorCode::PreconditionFailed`.
 */
class DATASERVICE_READ_API DataRequest final {
 public:
  /**
   * @brief Gets the ID of the requested partition.
   *
   * @return The partition ID.
   */
  const porting::optional<std::string>& GetPartitionId() const {
    return partition_id_;
  }

  /**
   * @brief Sets the partition ID.
   *
   * If the partition cannot be found in the layer, the callback returns
   * with an empty response (the `null` result for data and an error).
   *
   * @param partition_id The partition ID.
   *
   * @return A reference to the updated `DataRequest` instance.
   */
  template <class T = porting::optional<std::string>>
  DataRequest& WithPartitionId(T&& partition_id) {
    partition_id_ = std::forward<T>(partition_id);
    return *this;
  }

  /**
   * @brief Get the partition data handle.
   *
   * You can use the data handle to retrieve the data that relates to this
   * partition. The data handle identifies a specific blob so that you can
   * request the blob contents with the Blob API. When requesting data from
   * the Blob API, you must specify the catalog ID, layer ID, and data
   * handle.
   *
   * @return The partition data handle.
   */
  const porting::optional<std::string>& GetDataHandle() const {
    return data_handle_;
  }

  /**
   * @brief Sets the partition data handle.
   *
   * If the data handle cannot be found in the layer, the callback returns
   * with an empty response (the `null` result for data and an error).
   *
   * @see `GetDataHandle` for information on the partition data handle.
   *
   * @param data_handle The partition data handle.
   *
   * @return A reference to the updated `DataRequest` instance.
   */
  template <class T = porting::optional<std::string>>
  DataRequest& WithDataHandle(T&& data_handle) {
    data_handle_ = std::forward<T>(data_handle);
    return *this;
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
   * @param tag The `BillingTag` string or `olp::porting::none`.
   *
   * @return A reference to the updated `DataRequest` instance.
   */
  template <class T = porting::optional<std::string>>
  DataRequest& WithBillingTag(T&& tag) {
    billing_tag_ = std::forward<T>(tag);
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
  FetchOptions GetFetchOption() const { return fetch_option_; }

  /**
   * @brief Sets the fetch option that you can use to set the source from
   * which data should be fetched.
   *
   * @see `GetFetchOption()` for information on usage and format.
   *
   * @param fetch_option The `FetchOption` enum.
   *
   * @return A reference to the updated `DataRequest` instance.
   */
  DataRequest& WithFetchOption(FetchOptions fetch_option) {
    fetch_option_ = fetch_option;
    return *this;
  }

  /**
   * @brief Gets the request priority.
   *
   * The default priority is `Priority::NORMAL`.
   *
   * @return The request priority.
   */
  uint32_t GetPriority() const { return priority_; }

  /**
   * @brief Sets the priority of the request.
   *
   * @param priority The priority of the request.
   *
   * @return A reference to the updated `DataRequest` instance.
   */
  DataRequest& WithPriority(uint32_t priority) {
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
                        porting::optional<int64_t> version) const {
    std::stringstream out;
    out << layer_id << "[";
    if (GetPartitionId()) {
      out << *GetPartitionId();
    } else if (GetDataHandle()) {
      out << *GetDataHandle();
    }
    out << "]";
    if (version) {
      out << "@" << *version;
    }
    if (GetBillingTag()) {
      out << "$" << *GetBillingTag();
    }
    out << "^" << GetFetchOption();
    return out.str();
  }

 private:
  porting::optional<std::string> partition_id_;
  porting::optional<int64_t> catalog_version_;
  porting::optional<std::string> data_handle_;
  porting::optional<std::string> billing_tag_;
  FetchOptions fetch_option_{OnlineIfNotFound};
  uint32_t priority_{thread::NORMAL};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
