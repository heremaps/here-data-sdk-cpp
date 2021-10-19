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

#include "ProtectDependencyResolver.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include <olp/core/cache/CacheKeyGenerator.h>

namespace {
constexpr auto kQuadTreeDepth = 4u;
}  // namespace

namespace olp {
namespace dataservice {
namespace read {

ProtectDependencyResolver::ProtectDependencyResolver(
    const client::HRN& catalog, const std::string& layer_id, int64_t version,
    const client::OlpClientSettings& settings)
    : catalog_(catalog.ToCatalogHRNString()),
      layer_id_(layer_id),
      version_(version),
      partitions_cache_repository_(catalog, layer_id_, settings.cache),
      quad_trees_(),
      keys_to_protect_() {}

const cache::KeyValueCache::KeyListType&
ProtectDependencyResolver::GetKeysToProtect(const TileKeys& tiles) {
  keys_to_protect_.clear();
  for (const auto& tile : tiles) {
    auto it = FindQuad(tile);
    if (it != quad_trees_.end()) {
      // Quad tree for tile found. Get data handle for tile and add to list for
      // protection
      AddDataHandle(tile, it->second);
    } else {
      // find tile in cache
      ProcessTileKeyInCache(tile);
    }
  }
  return keys_to_protect_;
}

ProtectDependencyResolver::QuadsType::iterator
ProtectDependencyResolver::FindQuad(const geo::TileKey& tile_key) {
  auto max_depth = std::min<std::uint32_t>(tile_key.Level(), kQuadTreeDepth);
  for (auto i = 0u; i <= max_depth; ++i) {
    const auto& quad_root = tile_key.ChangedLevelBy(-i);
    auto it = quad_trees_.find(quad_root);
    if (it != quad_trees_.end()) {
      return it;
    }
  }
  return quad_trees_.end();
}

bool ProtectDependencyResolver::AddDataHandle(
    const geo::TileKey& tile, const read::QuadTreeIndex& quad_tree) {
  auto data = quad_tree.Find(tile, false);
  if (data) {
    keys_to_protect_.emplace_back(cache::CacheKeyGenerator::CreateDataHandleKey(
        catalog_, layer_id_, data->data_handle));
    return true;
  }
  return false;
}

bool ProtectDependencyResolver::ProcessTileKeyInCache(
    const geo::TileKey& tile) {
  read::QuadTreeIndex cached_tree;
  if (partitions_cache_repository_.FindQuadTree(tile, version_, cached_tree) &&
      AddDataHandle(tile, cached_tree)) {
    auto root_tile = cached_tree.GetRootTile();
    // add quad tree to list for protection
    keys_to_protect_.emplace_back(cache::CacheKeyGenerator::CreateQuadTreeKey(
        catalog_, layer_id_, root_tile, version_, kQuadTreeDepth));
    // save quad tree, because  there is could be more tiles to protect from
    // this quad
    quad_trees_[root_tile] = std::move(cached_tree);
    return true;
  }
  return false;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
