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

#include <map>
#include <memory>
#include <string>

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/geo/Types.h>
#include <olp/core/geo/tiling/TileKey.h>
#include "repositories/DataCacheRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/PartitionsCacheRepository.h"
#include "repositories/PartitionsRepository.h"

namespace olp {
namespace dataservice {
namespace read {

/// Find quad trees for protected tiles. Add quad tree to list for protection.
/// Save downloaded quads for future searches for tiles to protect, if quad tree
/// not found look it to the cache.
class ProtectDependencyResolver {
 public:
  ProtectDependencyResolver(const client::HRN& catalog,
                            const std::string& layer_id, int64_t version,
                            const client::OlpClientSettings& settings);

  const cache::KeyValueCache::KeyListType& GetKeysToProtect(
      const TileKeys& tiles);

 private:
  using QuadsType = std::map<geo::TileKey, read::QuadTreeIndex>;

  QuadsType::iterator FindQuad(const geo::TileKey& tile_key);

  bool AddDataHandle(const geo::TileKey& tile,
                     const read::QuadTreeIndex& quad_tree);

  bool ProcessTileKeyInCache(const geo::TileKey& tile);

 private:
  const std::string& layer_id_;
  const int64_t version_;
  repository::DataCacheRepository data_cache_repository_;
  repository::PartitionsCacheRepository partitions_cache_repository_;
  std::map<geo::TileKey, read::QuadTreeIndex> quad_trees_;
  cache::KeyValueCache::KeyListType keys_to_protect_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
