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

#include <string>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

class SubscribeResponse final {
 public:
  const std::string& GetNodeBaseURL() const {
    return node_base_url_;
  }
  void SetNodeBaseURL(std::string value) { node_base_url_ = std::move(value); }

  const std::string& GetSubscriptionId() const {
    return subscription_id_;
  }
  void SetSubscriptionId(std::string value) {
    subscription_id_ = std::move(value);
  }

 private:
  /// A subscription is created in a specific server instance. This base URL
  /// represents the specific server node on which this subscription was
  /// created. Use this base URL instead of the base URL returned by
  /// the API Lookup Service when you make further calls to the `StreamApi`
  /// methods.
  std::string node_base_url_;

  /// A unique identifier of the subscription. It is used to make further calls
  /// to the `StreamApi` methods.
  std::string subscription_id_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
