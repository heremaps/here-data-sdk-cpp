/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include <olp/core/cache/KeyGenerator.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/Atomic.h>
#include <olp/core/thread/TaskScheduler.h>
#include "ExtendedApiResponseHelpers.h"
#include "PartitionsRepository.h"
#include "QuadTreeIndex.h"
#include "generated/api/QueryApi.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "PrefetchTilesRepository";
constexpr std::int32_t kMaxQuadTreeIndexDepth = 4;

SubQuadsResult FlattenTree(const QuadTreeIndex& tree) {
  SubQuadsResult result;
  auto index_data = tree.GetIndexData(QuadTreeIndex::DataHandle);
  for (auto& data : index_data) {
    const auto it = result.lower_bound(data.tile_key);
    if (it == result.end() || result.key_comp()(data.tile_key, it->first)) {
      result.emplace_hint(it, std::piecewise_construct,
                          std::forward_as_tuple(data.tile_key),
                          std::forward_as_tuple(std::move(data.data_handle)));
    }
  }
  return result;
}

}  // namespace

PrefetchTilesRepository::PrefetchTilesRepository(
    client::HRN catalog, std::string layer_id,
    client::OlpClientSettings settings, client::ApiLookupClient client,
    boost::optional<std::string> billing_tag, NamedMutexStorage storage)
    : catalog_(std::move(catalog)),
      catalog_str_(catalog_.ToString()),
      layer_id_(std::move(layer_id)),
      settings_(std::move(settings)),
      lookup_client_(client),
      cache_repository_(catalog_, layer_id_, settings_.cache,
                        settings_.default_cache_expiration),
      billing_tag_(std::move(billing_tag)),
      storage_(std::move(storage)) {}

void PrefetchTilesRepository::SplitSubtree(
    RootTilesForRequest& root_tiles_depth,
    const RootTilesForRequest::iterator& subtree_to_split,
    const geo::TileKey& tile_key, std::uint32_t min) {
  unsigned int depth = subtree_to_split->second;
  auto tileKey = subtree_to_split->first;
  if (depth <= kMaxQuadTreeIndexDepth) {
    return;
  }
  while (depth > kMaxQuadTreeIndexDepth) {
    unsigned int level = depth - kMaxQuadTreeIndexDepth;
    auto childCount = geo::QuadKey64Helper::ChildrenAtLevel(level);

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
    if (tile_key.Level() < min_level) {
      min_level = tile_key.Level();
    }
    if (max_level == geo::TileKey::LevelCount) {
      max_level = min_level;
    } else {
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
        // if min_level is less than steps we need to go up, set min level to
        // zero. Some quads will overlap, but we will minimize quad tree
        // requests
        min_level = 0u;
      }
    }

    OLP_SDK_LOG_TRACE_F(
        kLogTag, "GetSlicedTiles for tile %s use min='%d', max='%d' levels",
        tile_key.ToHereTile().c_str(), min_level, max_level);

    // while adjusting levels, min_level could be changed only up
    // min_level is always less or equal to tile_key level
    // change root_tile for quad tree requests to be on min_level
    auto root_tile = tile_key.ChangedLevelTo(min_level);

    auto it = root_tiles_depth.insert({root_tile, max_level - min_level});
    // element already exist, update depth with max value
    if (!it.second) {
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

client::NetworkStatistics PrefetchTilesRepository::LoadAggregatedSubQuads(
    geo::TileKey root, const std::vector<geo::TileKey>& tiles,
    std::int64_t version, client::CancellationContext context) {
  // if quad tree isn't cached, no reason to download additional quads
  QuadTreeIndex quad_tree;
  client::NetworkStatistics network_stats;

  if (!cache_repository_.Get(root, kMaxQuadTreeIndexDepth, version,
                             quad_tree)) {
    return network_stats;
  }

  auto highest_tile_it = std::min_element(tiles.begin(), tiles.end());

  // Currently there is no better way to correctly handle the prefetch of
  // aggregated tiles, we download parent trees until the tile or it's parent is
  // found in subtiles. In this way we make sure that all tiles within requested
  // tree have aggregated parent downloaded and cached. This may cause
  // additional or duplicate download request.
  auto root_index = quad_tree.Find(*highest_tile_it, true);
  if (root_index) {
    const auto& aggregated_tile_key = root_index->tile_key;

    while (root.Level() > aggregated_tile_key.Level()) {
      root = root.ChangedLevelBy(-kMaxQuadTreeIndexDepth - 1);

      const auto quad_cache_key = cache::KeyGenerator::CreateQuadTreeKey(
          catalog_str_, layer_id_, root, version, kMaxQuadTreeIndexDepth);

      NamedMutex mutex(storage_, quad_cache_key, context);
      std::unique_lock<NamedMutex> lock(mutex);

      if (!cache_repository_.ContainsTree(root, kMaxQuadTreeIndexDepth,
                                          version)) {
        QuadTreeResponse response = DownloadVersionedQuadTree(
            root, kMaxQuadTreeIndexDepth, version, context);

        network_stats += GetNetworkStatistics(response);
      }
    }
  }

  return network_stats;
}

SubQuadsResponse PrefetchTilesRepository::GetVersionedSubQuads(
    geo::TileKey tile, int32_t depth, std::int64_t version,
    client::CancellationContext context) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetSubQuads(%s, %" PRId64 ", %" PRId32 ")",
                      tile.ToHereTile().c_str(), version, depth);
  // check if quad tree with requested tile and depth already in cache
  QuadTreeIndex quad_tree;
  client::NetworkStatistics network_stats;

  const auto quad_cache_key = cache::KeyGenerator::CreateQuadTreeKey(
      catalog_str_, layer_id_, tile, version, kMaxQuadTreeIndexDepth);

  NamedMutex mutex(storage_, quad_cache_key, context);
  std::lock_guard<NamedMutex> lock(mutex);

  if (cache_repository_.Get(tile, depth, version, quad_tree)) {
    OLP_SDK_LOG_TRACE_F(kLogTag,
                        "GetSubQuads found in cache, tile='%s', "
                        "depth='%" PRId32 "'",
                        tile.ToHereTile().c_str(), depth);
  } else {
    QuadTreeResponse response =
        DownloadVersionedQuadTree(tile, depth, version, context);

    network_stats = GetNetworkStatistics(response);

    if (!response.IsSuccessful()) {
      return {response.GetError(), network_stats};
    }

    quad_tree = response.MoveResult();
  }

  return {FlattenTree(quad_tree), network_stats};
}

SubQuadsResponse PrefetchTilesRepository::GetVolatileSubQuads(
    geo::TileKey tile, int32_t depth,
    const client::CancellationContext& context) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetSubQuadsVolatile(%s, %" PRId32 ")",
                      tile.ToHereTile().c_str(), depth);

  auto query_api = lookup_client_.LookupApi("query", "v1",
                                            client::OnlineIfNotFound, context);

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  auto tile_key = tile.ToHereTile();

  OLP_SDK_LOG_INFO_F(kLogTag, "GetSubQuadsVolatile execute(%s, %" PRId32 ")",
                     tile_key.c_str(), depth);

  auto quad_tree = QueryApi::QuadTreeIndexVolatile(
      query_api.GetResult(), layer_id_, tile_key, depth, boost::none,
      billing_tag_, context);

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

  OLP_SDK_LOG_TRACE_F(
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

  const auto put_result =
      cache_repository_.Put(partitions, boost::none, boost::none, false);
  if (!put_result.IsSuccessful()) {
    OLP_SDK_LOG_ERROR_F(kLogTag,
                        "GetVolatileSubQuads failed to write data to cache, "
                        "hrn='%s', key='%s', error=%s",
                        catalog_str_.c_str(), tile_key.c_str(),
                        put_result.GetError().GetMessage().c_str());
  }

  return result;
}

static bool skip_tile(const PrefetchTilesRequest& request,
                      const geo::TileKey& tile_key) {
  if (tile_key.Level() < request.GetMinLevel() ||
      tile_key.Level() > request.GetMaxLevel()) {
    return true;
  }
  for (const geo::TileKey& root_key : request.GetTileKeys()) {
    if (root_key == tile_key || root_key.IsParentOf(tile_key) ||
        tile_key.IsParentOf(root_key))
      return false;
  }
  return true;
}

void PrefetchTilesRepository::FilterTilesByLevel(
    const PrefetchTilesRequest& request, SubQuadsResult& tiles) const {
  for (auto sub_quad_it = tiles.begin(); sub_quad_it != tiles.end();) {
    if (skip_tile(request, sub_quad_it->first)) {
      sub_quad_it = tiles.erase(sub_quad_it);
    } else {
      ++sub_quad_it;
    }
  }
}

std::vector<geo::TileKey> PrefetchTilesRepository::FilterTileKeysByLevel(
    const PrefetchTilesRequest& request, const SubQuadsResult& tiles) const {
  std::vector<geo::TileKey> result;
  for (const auto& tile : tiles) {
    if (!skip_tile(request, tile.first)) {
      result.emplace_back(tile.first);
    }
  }
  return result;
}

void PrefetchTilesRepository::FilterTilesByList(
    const PrefetchTilesRequest& request, SubQuadsResult& tiles) const {
  SubQuadsResult result;

  const bool aggregation_enabled = request.GetDataAggregationEnabled();
  const auto& tile_keys = request.GetTileKeys();

  if (!aggregation_enabled) {
    for (const auto& tile : tile_keys) {
      const auto it = tiles.find(tile);
      auto& new_tile = result[tile];
      if (it != tiles.end()) {
        new_tile = std::move(it->second);
        tiles.erase(it);
      }
    }
  } else {
    auto append_tile = [&](const geo::TileKey& key) {
      const auto it = tiles.find(key);
      if (it != tiles.end()) {
        result.emplace(key, std::move(it->second));
        return true;
      } else {
        return result.find(key) != result.end();
      }
    };

    for (const auto& tile : tile_keys) {
      auto aggregated_tile = tile;

      while (aggregated_tile.IsValid() && !append_tile(aggregated_tile)) {
        aggregated_tile = aggregated_tile.Parent();
      }

      if (!aggregated_tile.IsValid()) {
        result[tile].clear();  // To generate Not Found error
      }
    }
  }
  tiles.swap(result);
}

std::vector<geo::TileKey> PrefetchTilesRepository::FilterTileKeysByList(
    const PrefetchTilesRequest& request, const SubQuadsResult& tiles) const {
  std::vector<geo::TileKey> result;

  if (!request.GetDataAggregationEnabled()) {
    result = request.GetTileKeys();
  } else {
    auto append_tile = [&tiles, &result](const geo::TileKey& key) {
      if (tiles.count(key) == 1) {
        result.emplace_back(key);
        return true;
      } else {
        return std::find(result.begin(), result.end(), key) != result.end();
      }
    };

    for (const auto& tile : request.GetTileKeys()) {
      auto aggregated_tile = tile;

      while (aggregated_tile.IsValid() && !append_tile(aggregated_tile)) {
        aggregated_tile = aggregated_tile.Parent();
      }

      if (!aggregated_tile.IsValid()) {
        result.emplace_back(tile);  // To generate Not Found error
      }
    }
  }
  return result;
}

PrefetchTilesRepository::QuadTreeResponse
PrefetchTilesRepository::DownloadVersionedQuadTree(
    geo::TileKey tile, int32_t depth, std::int64_t version,
    const client::CancellationContext& context) {
  static const std::vector<std::string> default_additional_fields = {
      PartitionsRequest::kChecksum, PartitionsRequest::kCrc,
      PartitionsRequest::kDataSize, PartitionsRequest::kCompressedDataSize};

  auto query_api = lookup_client_.LookupApi("query", "v1",
                                            client::OnlineIfNotFound, context);

  if (!query_api.IsSuccessful()) {
    return query_api.GetError();
  }

  auto tile_key = tile.ToHereTile();

  OLP_SDK_LOG_TRACE_F(kLogTag,
                      "GetSubQuads execute(%s, %" PRId64 ", %" PRId32 ")",
                      tile_key.c_str(), version, depth);

  auto quad_tree = QueryApi::QuadTreeIndex(
      query_api.GetResult(), layer_id_, tile_key, version, depth,
      default_additional_fields, billing_tag_, context);

  if (quad_tree.GetStatus() != olp::http::HttpStatusCode::OK) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "GetSubQuads failed(%s, %" PRId64 ", %" PRId32 "), status_code='%d'",
        tile_key.c_str(), version, depth, quad_tree.GetStatus());
    return {client::ApiError(quad_tree.GetStatus(),
                             quad_tree.GetResponseAsString()),
            quad_tree.GetNetworkStatistics()};
  }

  QuadTreeIndex tree(tile, depth, quad_tree.GetRawResponse());

  if (tree.IsNull()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "QuadTreeIndex failed, hrn='%s', "
                          "layer='%s', root='%s', version='%" PRId64
                          "', depth='%" PRId32 "'",
                          catalog_.ToString().c_str(), layer_id_.c_str(),
                          tile_key.c_str(), version, depth);
    return {client::ApiError::Unknown("Failed to parse quad tree response"),
            quad_tree.GetNetworkStatistics()};
  }

  // add to cache
  const auto put_result = cache_repository_.Put(tile, depth, tree, version);
  if (!put_result.IsSuccessful()) {
    return {put_result.GetError(), quad_tree.GetNetworkStatistics()};
  }

  return {std::move(tree), quad_tree.GetNetworkStatistics()};
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
