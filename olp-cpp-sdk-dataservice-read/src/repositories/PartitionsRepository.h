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

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include "MultiRequestContext.h"
#include "olp/dataservice/read/CatalogClient.h"

namespace olp {
namespace dataservice {
namespace read {
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
