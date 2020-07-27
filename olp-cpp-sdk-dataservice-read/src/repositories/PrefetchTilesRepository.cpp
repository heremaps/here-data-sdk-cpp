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

#include "PrefetchTilesRepository.h"

#include <inttypes.h>
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/Atomic.h>
#include <olp/core/thread/TaskScheduler.h>
#include "ApiClientLookup.h"
#include "PartitionsRepository.h"
#include "QuadTreeIndex.h"
#include "generated/api/QueryApi.h"

// Needed to avoid endless warnings from GetVersion/WithVersion
#include <olp/core/porting/warning_disable.h>
PORTING_PUSH_WARNINGS()
PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "PrefetchTilesRepository";
constexpr std::uint32_t kMaxQuadTreeIndexDepth = 4u;
}  // namespace

void PrefetchTilesRepository::SplitSubtree(
    RootTilesForRequest& root_tiles_depth,
    RootTilesForRequest::iterator subtree_to_split,
    const geo::TileKey& tile_key, std::uint32_t min) {
  unsigned int depth = subtree_to_split->second;
  auto tileKey = subtree_to_split->first;
  if (depth <= kMaxQuadTreeIndexDepth) {
    return;
  }
  while (depth > kMaxQuadTreeIndexDepth) {
    unsigned int level = depth - kMaxQuadTreeIndexDepth;
    int childCount = geo::QuadKey64Helper::ChildrenAtLevel(level);

    const geo::TileKey firstChild =
        tileKey.ChangedLevelTo(tileKey.Level() + level);
    const std::uint64_t beginTileKey = firstChild.ToQuadKey64();

    const std::uint64_t endTileKey = beginTileKey + childCount;

    for (std::uint64_t key = beginTileKey; key < endTileKey; ++key) {
      auto child = geo::TileKey::FromQuadKey64(key);
      // skip child, if it is not a parent, or a child of prefetched tile
      if ((!tile_key.IsParentOf(child) && !child.IsParentOf(tile_key) &&
           child != tile_key) ||
          (child.Level() + kMaxQuadTreeIndexDepth < min)) {
        continue;
      }
      root_tiles_depth.insert({child, kMaxQuadTreeIndexDepth});
    }
    depth -= (kMaxQuadTreeIndexDepth + 1);
  }
  if (tileKey.Level() + depth < min) {
    root_tiles_depth.erase(subtree_to_split);
  } else {
    subtree_to_split->second = depth;
  }
}

RootTilesForRequest PrefetchTilesRepository::GetSlicedTiles(
    const std::vector<geo::TileKey>& tile_keys, std::uint32_t min,
    std::uint32_t max) {
  RootTilesForRequest root_tiles_depth;
  // adjust root tiles to min level
  for (const auto& tile_key : tile_keys) {
    // for each tile adjust its own min/max levels needed query quad tree index
    auto min_level = min;
    auto max_level = max;

    // min level should always start from level of root tile for quad tree
    // index, if requested tile is on upper level, adjust min_level up
    if (tile_key.Level() < min_level || min_level == 0) {
      min_level = tile_key.Level();
      max_level = std::max(max_level, min_level);
    }

    // adjust min/max levels, if distance between min/max could not be splitted
    // with depth 4
    auto extra_levels =
        (max_level + 1u - min_level) % (kMaxQuadTreeIndexDepth + 1);
    if (extra_levels != 0) {
      // calculate how many levels up we should change min level
      auto levels_up = kMaxQuadTreeIndexDepth + 1 - extra_levels;
      if (min_level > levels_up) {
        min_level = min_level - levels_up;
      } else {
        // if min_level is less than steps we need to go up, go some steps down
        auto levels_down = levels_up - min_level + 1u;
        min_level = 1u;
        max_level = max_level + levels_down;
      }
    }

    OLP_SDK_LOG_DEBUG_F(
        kLogTag, "GetSlicedTiles for tile %s use min='%d', max='%d' levels",
        tile_key.ToHereTile().c_str(), min_level, max_level);

    // while adjusting levels, min_level could be changed only up
    // min_level is always less or equal to tile_key level
    // change root_tile for quad tree requests to be on min_level
    auto root_tile = tile_key.ChangedLevelTo(min_level);

    auto it = root_tiles_depth.insert({root_tile, max_level - min_level});
    // element already exist, update depth with max value
    if (it.second == false) {
      it.first->second = std::max(it.first->second, max_level - min_level);
    }
    // if depth is greater than kMaxQuadTreeIndexDepth, need to split
    if (it.first->second > kMaxQuadTreeIndexDepth) {
      // split subtree on chunks with depth kMaxQuadTreeIndexDepth
      SplitSubtree(root_tiles_depth, it.first, tile_key, min);
    }
  }
  return root_tiles_depth;
}

SubTilesResponse PrefetchTilesRepository::GetSubTiles(
    const client::HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, boost::optional<std::int64_t> version,
    const RootTilesForRequest& root_tiles, client::CancellationContext context,
    const client::OlpClientSettings& settings) {
  SubTilesResult result;
  OLP_SDK_LOG_INFO_F(kLogTag,
                     "GetSubTiles: hrn='%s', layer='%s', root_tiles=%zu",
                     catalog.ToCatalogHRNString().c_str(), layer_id.c_str(),
                     root_tiles.size());

  for (const auto& quad : root_tiles) {
    if (context.IsCancelled()) {
      return {{client::ErrorCode::Cancelled, "Cancelled", true}};
    }

    auto& tile = quad.first;
    auto& depth = quad.second;
    auto response = version
                        ? GetSubQuads(catalog, layer_id, request, version.get(),
                                      tile, depth, settings, context)
                        : GetVolatileSubQuads(catalog, layer_id, request, tile,
                                              depth, settings, context);
    if (!response.IsSuccessful()) {
      // Just abort if something else then 404 Not Found is returned
      auto& error = response.GetError();
      if (error.GetHttpStatusCode() != http::HttpStatusCode::NOT_FOUND) {
        return error;
      }
    }
    auto subtiles = response.MoveResult();
    result.insert(std::make_move_iterator(subtiles.begin()),
                  std::make_move_iterator(subtiles.end()));
  }
  return result;
}

SubQuadsResponse PrefetchTilesRepository::GetSubQuads(
    const client::HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, std::int64_t version,
    geo::TileKey tile, int32_t depth, const client::OlpClientSettings& settings,
    client::CancellationContext context) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetSubQuads(%s, %" PRId64 ", %" PRId32 ")",
                      tile.ToHereTile().c_str(), version, depth);

  repository::PartitionsCacheRepository repository(
      catalog, settings.cache, settings.default_cache_expiration);

  auto get_sub_quads = [](const QuadTreeIndex& tree) -> SubQuadsResult {
    SubQuadsResult result;
    auto index_data = tree.GetIndexData();
    std::transform(
        index_data.begin(), index_data.end(),
        std::inserter(result, result.end()),
        [](const QuadTreeIndex::IndexData& data) -> SubQuadsResult::value_type {
          return {data.tile_key, data.data_handle};
        });
    return result;
  };

  // check if quad tree with requested tile and depth already in cache
  QuadTreeIndex cached_tree;
  if (repository.Get(layer_id, tile, depth, version, cached_tree)) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "GetSubQuads found in cache, tile='%s', "
                        "depth='%" PRId32 "'",
                        tile.ToHereTile().c_str(), depth);

    return get_sub_quads(cached_tree);
  }

  auto query_api =
      ApiClientLookup::LookupApi(catalog, context, "query", "v1",
                                 FetchOptions::OnlineIfNotFound, settings);

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  auto tile_key = tile.ToHereTile();

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "GetSubQuads execute(%s, %" PRId64 ", %" PRId32 ")",
                     tile_key.c_str(), version, depth);

  auto quad_tree = QueryApi::QuadTreeIndex(
      query_api.GetResult(), layer_id, tile_key, version, depth, boost::none,
      request.GetBillingTag(), context);

  if (quad_tree.status != olp::http::HttpStatusCode::OK) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetSubQuads failed(%s, %" PRId64 ", %" PRId32 ")",
                          tile_key.c_str(), version, depth);
    return {{quad_tree.status, quad_tree.response.str()}};
  }

  QuadTreeIndex tree(tile, depth, quad_tree.response);

  if (tree.IsNull()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "QuadTreeIndex failed, hrn='%s', "
                          "layer='%s', root='%s', version='%" PRId64
                          "', depth='%" PRId32 "'",
                          catalog.ToString().c_str(), layer_id.c_str(),
                          tile_key.c_str(), version, depth);
    return {{client::ErrorCode::Unknown, "Failed to parse quad tree response"}};
  }
  // add to cache
  repository.Put(layer_id, tile, depth, tree, version);
  return get_sub_quads(tree);
}

SubQuadsResponse PrefetchTilesRepository::GetVolatileSubQuads(
    const client::HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, geo::TileKey tile, int32_t depth,
    const client::OlpClientSettings& settings,
    client::CancellationContext context) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetSubQuadsVolatile(%s, %" PRId32 ")",
                      tile.ToHereTile().c_str(), depth);

  auto query_api =
      ApiClientLookup::LookupApi(catalog, context, "query", "v1",
                                 FetchOptions::OnlineIfNotFound, settings);

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  using QuadTreeIndexResponse = QueryApi::QuadTreeIndexResponse;
  using QuadTreeIndexPromise = std::promise<QuadTreeIndexResponse>;
  auto promise = std::make_shared<QuadTreeIndexPromise>();
  auto tile_key = tile.ToHereTile();

  OLP_SDK_LOG_INFO_F(kLogTag, "GetSubQuadsVolatile execute(%s, %" PRId32 ")",
                     tile_key.c_str(), depth);

  auto quad_tree = QueryApi::QuadTreeIndexVolatile(
      query_api.GetResult(), layer_id, tile_key, depth, boost::none,
      request.GetBillingTag(), context);

  if (!quad_tree.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetSubQuadsVolatile failed(%s, %" PRId32 ")",
                          tile_key.c_str(), depth);
    return quad_tree.GetError();
  }

  SubQuadsResult result;
  model::Partitions partitions;

  const auto& subquads = quad_tree.GetResult().GetSubQuads();
  partitions.GetMutablePartitions().reserve(subquads.size());

  OLP_SDK_LOG_DEBUG_F(
      kLogTag, "GetSubQuad finished, key=%s, size=%zu, depth=%" PRId32 ")",
      tile_key.c_str(), subquads.size(), depth);

  for (const auto& subquad : subquads) {
    auto subtile = tile.AddedSubHereTile(subquad->GetSubQuadKey());

    // Add to result
    result.emplace(subtile, subquad->GetDataHandle());

    // add to bulk partitions for cacheing
    partitions.GetMutablePartitions().emplace_back(
        PartitionsRepository::PartitionFromSubQuad(*subquad,
                                                   subtile.ToHereTile()));
  }

  // add to cache
  repository::PartitionsCacheRepository cache(
      catalog, settings.cache, settings.default_cache_expiration);
  cache.Put({}, partitions, layer_id, boost::none, false);

  return result;
}

SubQuadsResult PrefetchTilesRepository::FilterSkippedTiles(
    const PrefetchTilesRequest& request, bool request_only_input_tiles,
    SubQuadsResult sub_tiles) {
  auto skip_tile = [&](const geo::TileKey& tile_key) {
    const auto& tile_keys = request.GetTileKeys();
    if (request_only_input_tiles) {
      return std::find(tile_keys.begin(), tile_keys.end(), tile_key) ==
             tile_keys.end();
    } else if (tile_key.Level() < request.GetMinLevel() ||
               tile_key.Level() > request.GetMaxLevel()) {
      // tile outside min/max segment, skip this tile
      return true;
    } else {
      // tile is not a parent or child for any of requested tiles, skip
      // this tile
      return std::find_if(tile_keys.begin(), tile_keys.end(),
                          [&tile_key](const geo::TileKey& root_key) {
                            return (root_key.IsParentOf(tile_key) ||
                                    tile_key.IsParentOf(root_key) ||
                                    root_key == tile_key);
                          }) == tile_keys.end();
    }
  };

  for (auto sub_quad_it = sub_tiles.begin(); sub_quad_it != sub_tiles.end();) {
    if (skip_tile(sub_quad_it->first)) {
      sub_quad_it = sub_tiles.erase(sub_quad_it);
    } else {
      ++sub_quad_it;
    }
  }
  return sub_tiles;
}

PORTING_POP_WARNINGS()
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
