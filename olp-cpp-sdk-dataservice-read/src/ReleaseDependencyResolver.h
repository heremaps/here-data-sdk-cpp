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

/// group quad trees and protected tiles. Check if for each quad tree we want
/// to release all protected keys associated, if true add this quad tree to
/// released keys
class ReleaseDependencyResolver {
 public:
  ReleaseDependencyResolver(const client::HRN& catalog,
                            const std::string& layer_id, const int64_t& version,
                            const client::OlpClientSettings& settings);

  const cache::KeyValueCache::KeyListType& GetKeysToRelease(
      const TileKeys& tiles);

 private:
  using TilesDataKeysType = std::map<geo::TileKey, std::string>;
  using QuadsType = std::map<geo::TileKey, TilesDataKeysType>;

  bool ProcessTileKey(const geo::TileKey& tile_key);

  QuadsType::iterator FindQuad(const geo::TileKey& tile_key);

  TilesDataKeysType ProcessQuad(const read::QuadTreeIndex& cached_tree,
                                const geo::TileKey& tile);

  void ProcessTileKeyInCache(const geo::TileKey& tile);

 private:
  const std::string& layer_id_;
  const int64_t version_;
  std::shared_ptr<cache::KeyValueCache> cache_;
  repository::DataCacheRepository data_cache_repository_;
  repository::PartitionsCacheRepository partitions_cache_repository_;
  QuadsType quad_trees_with_protected_tiles_;
  cache::KeyValueCache::KeyListType keys_to_release_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
