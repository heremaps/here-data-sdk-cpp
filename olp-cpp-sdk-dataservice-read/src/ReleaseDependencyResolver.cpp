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

#include "ReleaseDependencyResolver.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

namespace {
constexpr auto kQuadTreeDepth = 4;
}  // namespace

namespace olp {
namespace dataservice {
namespace read {

cache::KeyValueCache::KeyListType
ReleaseDependencyResolver::GenerateKeysToRelease(
    const client::HRN& catalog, const std::string& layer_id,
    const int64_t& version, const client::OlpClientSettings& settings,
    const TileKeys& tiles) {
  auto process_tiles =
      ReleaseDependencyResolver(catalog, layer_id, version, settings);
  return process_tiles.GenerateAndMoveKeysToRelease(tiles);
}

ReleaseDependencyResolver::ReleaseDependencyResolver(
    const client::HRN& catalog, const std::string& layer_id,
    const int64_t& version, const client::OlpClientSettings& settings)
    : layer_id_(layer_id),
      version_(version),
      cache_(settings.cache),
      data_cache_repository_(catalog, settings.cache),
      partitions_cache_repository_(catalog, settings.cache),
      repository_(catalog, settings),
      quad_trees_with_protected_tiles_(),
      keys_to_release_() {}

cache::KeyValueCache::KeyListType&&
ReleaseDependencyResolver::GenerateAndMoveKeysToRelease(const TileKeys& tiles) {
  for (const auto& tile : tiles) {
    if (!ProcessTileKeyInMap(tile)) {
      ProcessTileKeyInCache(tile);
    }
  }
  return std::move(keys_to_release_);
}

bool ReleaseDependencyResolver::ProcessTileKeyInMap(
    const geo::TileKey& tile_key) {
  auto it = FindQuadInMap(tile_key);
  if (it != quad_trees_with_protected_tiles_.end()) {
    // Quad tree for tile found. Get data handle for tile and add to list to
    // release
    auto tile_it = it->second.find(tile_key);
    // if tile_key_to_release was not found, this means it is not
    // protected
    if (tile_it != it->second.end()) {
      keys_to_release_.emplace_back(tile_it->second);
      // key added, we can remove this tile from map
      it->second.erase(tile_it);
      if (it->second.empty()) {
        // no more protected tiles associated with this quad tree
        // can add key for quad tree to be released and remove from map
        keys_to_release_.emplace_back(
            partitions_cache_repository_.CreateQuadKey(
                layer_id_, it->first, kQuadTreeDepth, version_));
        quad_trees_with_protected_tiles_.erase(it);
      }
    }
    return true;
  }
  return false;
}

ReleaseDependencyResolver::MapOfQuads::iterator
ReleaseDependencyResolver::FindQuadInMap(const geo::TileKey& tile_key) {
  auto max_depth = std::min<std::int32_t>(tile_key.Level(), kQuadTreeDepth);
  for (auto i = max_depth; i >= 0; --i) {
    const auto& quad_root = tile_key.ChangedLevelBy(-i);
    auto it = quad_trees_with_protected_tiles_.find(quad_root);
    if (it != quad_trees_with_protected_tiles_.end()) {
      return it;
    }
  }
  return quad_trees_with_protected_tiles_.end();
}

ReleaseDependencyResolver::MapOfTileDataKeys
ReleaseDependencyResolver::ProcessQuad(const read::QuadTreeIndex& cached_tree,
                                       const geo::TileKey& tile) {
  // check if quad tree has other protected keys, if not, add quad key to
  // release from protected list, othervise add all protected keys left
  // for this quad to map
  auto index_data = cached_tree.GetIndexData();
  MapOfTileDataKeys protected_keys;
  for (const auto& ind : index_data) {
    auto tile_data_key =
        data_cache_repository_.CreateKey(layer_id_, ind.data_handle);
    if (cache_->IsProtected(tile_data_key)) {
      if (ind.tile_key == tile) {
        // add key to release list
        keys_to_release_.emplace_back(tile_data_key);
      } else {
        // add key to map for future searches of released tiles for this
        // quad tree
        protected_keys[ind.tile_key] = tile_data_key;
      }
    }
  }
  return protected_keys;
}

void ReleaseDependencyResolver::ProcessTileKeyInCache(
    const geo::TileKey& tile) {
  read::QuadTreeIndex cached_tree;
  if (repository_.FindQuadTree(layer_id_, version_, tile, cached_tree)) {
    MapOfTileDataKeys protected_keys = ProcessQuad(cached_tree, tile);
    if (protected_keys.empty()) {
      // no other tiles are protected, can add quad tree to release list
      keys_to_release_.emplace_back(partitions_cache_repository_.CreateQuadKey(
          layer_id_, cached_tree.GetRootTile(), kQuadTreeDepth, version_));
    } else {
      // add quad key with other protected keys dependent on this quad to
      // reduce future calls to cache
      quad_trees_with_protected_tiles_[cached_tree.GetRootTile()] =
          std::move(protected_keys);
    }
  }
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
