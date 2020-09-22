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

#include <memory>
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <boost/optional.hpp>
#include "generated/model/Partitions.h"

namespace olp {
namespace client {
class OlpClient;
}

namespace dataservice {
namespace write {

/**
 * @brief Api to get information about layers and partitions stored in a
 * catalog.
 */
class QueryApi {
 public:
  using PartitionsResponse =
      client::ApiResponse<model::Partitions, client::ApiError>;
  using PartitionsCallback = std::function<void(PartitionsResponse)>;

  static client::CancellationToken GetPartitionsById(
      const client::OlpClient& client, const std::string& layerId,
      const std::vector<std::string>& partitionIds,
      boost::optional<int64_t> version,
      boost::optional<std::vector<std::string>> additionalFields,
      boost::optional<std::string> billingTag,
      const PartitionsCallback& callback);

  static PartitionsResponse GetPartitionsById(
      const client::OlpClient& client, const std::string& layerId,
      const std::vector<std::string>& partition_ids,
      boost::optional<int64_t> version,
      boost::optional<std::vector<std::string>> additional_fields,
      boost::optional<std::string> billing_tag,
      client::CancellationContext context);
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
