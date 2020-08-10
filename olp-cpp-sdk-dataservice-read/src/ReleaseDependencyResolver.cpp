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

ReleaseDependencyResolver::ReleaseDependencyResolver(
    const client::HRN& catalog, const std::string& layer_id, int64_t version,
    const client::OlpClientSettings& settings)
    : layer_id_(layer_id),
      version_(version),
      cache_(settings.cache),
      data_cache_repository_(catalog, settings.cache),
      partitions_cache_repository_(catalog, settings.cache),
      quad_trees_with_protected_tiles_(),
      keys_to_release_() {}

const cache::KeyValueCache::KeyListType&
ReleaseDependencyResolver::GetKeysToRelease(const TileKeys& tiles) {
  keys_to_release_.clear();
  for (const auto& tile : tiles) {
    ProcessTileKey(tile);
  }
  return keys_to_release_;
}

void ReleaseDependencyResolver::ProcessTileKey(const geo::TileKey& tile_key) {
  bool add_key = true;
  auto process_tile = [&](const geo::TileKey& quad_root,
                          const geo::TileKey& tile) {
    auto it = quad_trees_with_protected_tiles_.find(quad_root);
    if (it != quad_trees_with_protected_tiles_.end()) {
      // Quad tree for tile found. Get data handle for tile and add to list
      // to release
      auto tile_it = it->second.find(tile);
      // if tile_key_to_release was not found, this means it is not
      // protected
      if (tile_it == it->second.end()) {
        return true;
      }
      if (add_key) {
        keys_to_release_.emplace_back(tile_it->second);
        add_key = false;
      }
      // key added, we can remove this tile from map
      it->second.erase(tile_it);
      if (it->second.empty()) {
        // no more protected tiles associated with this quad tree
        // can add key for quad tree to be released and remove from map
        keys_to_release_.emplace_back(
            partitions_cache_repository_.CreateQuadKey(
                layer_id_, it->first, kQuadTreeDepth, version_));
      }
      return true;
    }
    return false;
  };

  auto max_depth = std::min<std::int32_t>(tile_key.Level(), kQuadTreeDepth);
  for (auto i = max_depth; i >= 0; --i) {
    const auto& quad_root = tile_key.ChangedLevelBy(-i);
    // we found quad that can contain our tile
    if (!process_tile(quad_root, tile_key)) {
      // load quad_root from cache
      ProcessQuadTreeCache(quad_root, tile_key, add_key);
    }
  }
}

ReleaseDependencyResolver::TilesDataKeysType
ReleaseDependencyResolver::CheckProtectedTilesInQuad(
    const read::QuadTreeIndex& cached_tree, const geo::TileKey& tile,
    bool& add_data_handle_key) {
  // check if quad tree has other protected keys, if not, add quad key to
  // release from protected list, othervise add all protected keys left
  // for this quad to map
  auto index_data = cached_tree.GetIndexData();
  TilesDataKeysType protected_keys;
  for (const auto& ind : index_data) {
    auto tile_data_key =
        data_cache_repository_.CreateKey(layer_id_, ind.data_handle);
    if (cache_->IsProtected(tile_data_key)) {
      if (ind.tile_key == tile) {
        // add key to release list
        if (add_data_handle_key) {
          keys_to_release_.emplace_back(tile_data_key);
          add_data_handle_key = false;
        }
      } else {
        // add key to map for future searches of released tiles for this
        // quad tree
        protected_keys[ind.tile_key] = tile_data_key;
      }
    }
  }
  return protected_keys;
}

void ReleaseDependencyResolver::ProcessQuadTreeCache(
    const geo::TileKey& root_quad_key, const geo::TileKey& tile,
    bool& add_key) {
  QuadTreeIndex cached_tree;
  if (partitions_cache_repository_.Get(layer_id_, root_quad_key, kQuadTreeDepth,
                                       version_, cached_tree)) {
    TilesDataKeysType protected_keys =
        CheckProtectedTilesInQuad(cached_tree, tile, add_key);
    if (protected_keys.empty()) {
      // no other tiles are protected, can add quad tree to release list
      keys_to_release_.emplace_back(partitions_cache_repository_.CreateQuadKey(
          layer_id_, root_quad_key, kQuadTreeDepth, version_));
    }
    // add quad key with other protected keys dependent on this quad to
    // reduce future calls to cache
    quad_trees_with_protected_tiles_[root_quad_key] = std::move(protected_keys);
  } else {
    // add empty TilesDataKeysType to know that we already loaded this root from
    // cache
    quad_trees_with_protected_tiles_[root_quad_key] = TilesDataKeysType();
  }
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
