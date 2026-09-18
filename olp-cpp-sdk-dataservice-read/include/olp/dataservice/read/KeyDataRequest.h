/*
 * Copyright (C) 2026 HERE Europe B.V.
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

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Encapsulates the fields required to request a plain data blob for
 * the given catalog, layer, and key.
 *
 * You must specify the key of the blob to be retrieved. If the key is not
 * set in the request, the request fails with the following error:
 * `ErrorCode::InvalidArgument`.
 */
class DATASERVICE_READ_API KeyDataRequest final {
 public:
  /**
   * @brief Gets the key of the requested data blob.
   *
   * @return The key.
   */
  const porting::optional<std::string>& GetKey() const { return key_; }

  /**
   * @brief Sets the key of the data blob to be retrieved.
   *
   * If the key cannot be found in the layer, the callback returns with an
   * empty response (the `null` result for data and an error).
   *
   * @param key The key.
   *
   * @return A reference to the updated `KeyDataRequest` instance.
   */
  template <class T = porting::optional<std::string>>
  KeyDataRequest& WithKey(T&& key) {
    key_ = std::forward<T>(key);
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
   * @return A reference to the updated `KeyDataRequest` instance.
   */
  template <class T = porting::optional<std::string>>
  KeyDataRequest& WithBillingTag(T&& tag) {
    billing_tag_ = std::forward<T>(tag);
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
   * @return A reference to the updated `KeyDataRequest` instance.
   */
  KeyDataRequest& WithPriority(uint32_t priority) {
    priority_ = priority;
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
    out << layer_id << "[";
    if (GetKey()) {
      out << *GetKey();
    }
    out << "]";
    if (GetBillingTag()) {
      out << "$" << *GetBillingTag();
    }
    return out.str();
  }

 private:
  porting::optional<std::string> key_;
  porting::optional<std::string> billing_tag_;
  uint32_t priority_{thread::NORMAL};
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
