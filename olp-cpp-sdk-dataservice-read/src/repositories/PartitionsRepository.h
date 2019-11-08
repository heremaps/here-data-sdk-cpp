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

#include <string>
#include <vector>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include "MultiRequestContext.h"
#include "generated/api/QueryApi.h"
#include "olp/dataservice/read/Types.h"

namespace olp {
namespace dataservice {
namespace read {
class DataRequest;
class PartitionsRequest;

namespace repository {
class ApiRepository;
class CatalogRepository;
class PartitionsCacheRepository;

class PartitionsRepository final {
 public:
  PartitionsRepository(const client::HRN& hrn,
                       std::shared_ptr<ApiRepository> apiRepo,
                       std::shared_ptr<CatalogRepository> catalogRepo,
                       std::shared_ptr<cache::KeyValueCache> cache_);

  ~PartitionsRepository() = default;

  client::CancellationToken GetPartitions(
      const read::PartitionsRequest& request,
      const read::PartitionsResponseCallback& callback);

  client::CancellationToken GetPartitionsById(
      const read::PartitionsRequest& request,
      const std::vector<std::string>& partitions,
      const read::PartitionsResponseCallback& callback);

  std::shared_ptr<PartitionsCacheRepository> GetPartitionsCacheRepository() {
    return cache_;
  }
  
  static PartitionsResponse GetVersionedPartitions(
      const client::HRN& catalog, const std::string& layer,
      client::CancellationContext cancellation_context,
      read::PartitionsRequest data_request, client::OlpClientSettings settings);

  static PartitionsResponse GetPartitions(
      const client::HRN& catalog, const std::string& layer,
      client::CancellationContext cancellation_context,
      read::PartitionsRequest request, client::OlpClientSettings settings);

  static QueryApi::PartitionsResponse GetPartitionById(
      const client::HRN& catalog, const std::string& layer,
      client::CancellationContext cancellation_context,
      const DataRequest& data_request, client::OlpClientSettings settings);

 private:
  client::HRN hrn_;
  std::shared_ptr<ApiRepository> apiRepo_;
  std::shared_ptr<CatalogRepository> catalogRepo_;
  std::shared_ptr<PartitionsCacheRepository> cache_;
  std::shared_ptr<MultiRequestContext<read::PartitionsResponse>>
      multiRequestContext_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
