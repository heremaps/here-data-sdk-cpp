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

#include <olp/core/client/HRN.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/model/Partitions.h>
#include <boost/optional.hpp>
#include "generated/model/LayerVersions.h"

namespace olp {
namespace cache {
class KeyValueCache;
}
namespace dataservice {
namespace read {
namespace repository {

class PartitionsCacheRepository final {
 public:
  PartitionsCacheRepository(const client::HRN& hrn,
                            std::shared_ptr<cache::KeyValueCache> cache);

  ~PartitionsCacheRepository() = default;

  void Put(const PartitionsRequest& request,
           const model::Partitions& partitions,
           const boost::optional<time_t>& expiry, bool allLayer = false);

  model::Partitions Get(const PartitionsRequest& request,
                        const std::vector<std::string>& partitionIds);

  boost::optional<model::Partitions> Get(const PartitionsRequest& request);

  void Put(int64_t catalogVersion, const model::LayerVersions& layerVersions);

  boost::optional<model::LayerVersions> Get(int64_t catalogVersion);

  void Clear(const std::string& layer_id);

  void ClearPartitions(const PartitionsRequest& request,
                       const std::vector<std::string>& partitionIds);


 private:
  client::HRN hrn_;
  std::shared_ptr<cache::KeyValueCache> cache_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
