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

#include <chrono>
#include <memory>

#include <olp/core/client/HRN.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/model/Partitions.h>
#include <boost/optional.hpp>
#include "QuadTreeIndex.h"
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
  PartitionsCacheRepository(
      const client::HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache,
      std::chrono::seconds default_expiry = std::chrono::seconds::max());

  ~PartitionsCacheRepository() = default;

  void Put(const model::Partitions& partitions, const std::string& layer_id,
           const boost::optional<int64_t>& version,
           const boost::optional<time_t>& expiry, bool layer_metadata = false);

  model::Partitions Get(const std::vector<std::string>& partition_ids,
                        const std::string& layer_id,
                        const boost::optional<int64_t>& version);

  boost::optional<model::Partitions> Get(
      const PartitionsRequest& request, const std::string& layer_id,
      const boost::optional<int64_t>& version);

  void Put(int64_t catalog_version, const model::LayerVersions& layer_versions);

  boost::optional<model::LayerVersions> Get(int64_t catalog_version);

  void Put(const std::string& layer, geo::TileKey tile_key, int32_t depth,
           const QuadTreeIndex& quad_tree,
           const boost::optional<int64_t>& version);

  bool Get(const std::string& layer, geo::TileKey tile_key, int32_t depth,
           const boost::optional<int64_t>& version, QuadTreeIndex& tree);

  void Clear(const std::string& layer_id);

  void ClearPartitions(const std::vector<std::string>& partition_ids,
                       const std::string& layer_id,
                       const boost::optional<int64_t>& version);

  bool ClearQuadTree(const std::string& layer, geo::TileKey tile_key,
                     int32_t depth, const boost::optional<int64_t>& version);

  bool ClearPartitionMetadata(const boost::optional<int64_t>& catalog_version,
                              const std::string& partition_id,
                              const std::string& layer_id,
                              boost::optional<model::Partition>& out_partition);

  bool GetPartitionHandle(const boost::optional<int64_t>& catalog_version,
                          const std::string& partition_id,
                          const std::string& layer_id,
                          std::string& data_handle);

  std::string CreateQuadKey(const std::string& layer, olp::geo::TileKey key,
                            int32_t depth,
                            const boost::optional<int64_t>& version) const;

  bool FindQuadTree(const std::string& layer, boost::optional<int64_t> version,
                    const olp::geo::TileKey& tile_key,
                    read::QuadTreeIndex& tree);

 private:
  client::HRN hrn_;
  std::shared_ptr<cache::KeyValueCache> cache_;
  time_t default_expiry_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
