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
#include <vector>

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/Atomic.h>
#include <olp/core/thread/TaskScheduler.h>
#include "ApiClientLookup.h"
#include "PartitionsRepository.h"
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
    RootTilesForRequest::iterator subtree_to_split) {
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
      auto it = root_tiles_depth.insert(
          {geo::TileKey::FromQuadKey64(key), kMaxQuadTreeIndexDepth});
      // element already exist, update depth with max value(should never hapen)
      if (it.second == false) {
        it.first->second = kMaxQuadTreeIndexDepth;
      }
    }
    depth -= (kMaxQuadTreeIndexDepth + 1);
  }
  subtree_to_split->second = depth;
}

RootTilesForRequest PrefetchTilesRepository::GetSlicedTiles(
    const std::vector<geo::TileKey>& tile_keys, unsigned int min,
    unsigned int max) {
  RootTilesForRequest root_tiles_depth;
  // adjust root tiles to min level
  for (auto tile_key : tile_keys) {
    unsigned int min_level = min;
    unsigned int max_level = max;

    if (max_level == min_level) {
      if (max_level == 0) {  // special case, adjust levels to input tile level
        max_level = tile_key.Level();
        min_level = (max_level > kMaxQuadTreeIndexDepth)
                        ? (max_level - kMaxQuadTreeIndexDepth)
                        : (1u);
      } else {
        // min max values are the same
        // to reduce methadata requests, go 4 levels up
        min_level = (min_level > kMaxQuadTreeIndexDepth)
                        ? (min_level - kMaxQuadTreeIndexDepth)
                        : (1u);
      }
    }

    // go up till min level is reached
    while (tile_key.Level() >= min_level && tile_key.IsValid()) {
      auto parent = tile_key.Parent();
      if (tile_key.Level() == min_level || !parent.IsValid()) {
        auto it = root_tiles_depth.insert({tile_key, max_level - min_level});
        // element already exist, update depth with max value
        if (it.second == false) {
          it.first->second = std::max(it.first->second, max_level - min_level);
        }
        // if depth is greater than kMaxQuadTreeIndexDepth, need to split
        if (it.first->second > kMaxQuadTreeIndexDepth) {
          // split subtree on chunks with depth kMaxQuadTreeIndexDepth
          SplitSubtree(root_tiles_depth, it.first);
        }
      }
      tile_key = parent;
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

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "GetSubQuads execute(%s, %" PRId64 ", %" PRId32 ")",
                     tile_key.c_str(), version, depth);

  auto quad_tree = QueryApi::QuadTreeIndex(
      query_api.GetResult(), layer_id, version, tile_key, depth, boost::none,
      request.GetBillingTag(), context);

  if (!quad_tree.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetSubQuads failed(%s, %" PRId64 ", %" PRId32 ")",
                          tile_key.c_str(), version, depth);
    return quad_tree.GetError();
  }

  SubQuadsResult result;
  model::Partitions partitions;

  const auto& subquads = quad_tree.GetResult().GetSubQuads();
  partitions.GetMutablePartitions().reserve(subquads.size());

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "GetSubQuad finished, key=%s, size=%zu,  version=%" PRId64
                      ", depth=%" PRId32 ")",
                      tile_key.c_str(), subquads.size(), version, depth);

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
  cache.Put(PartitionsRequest().WithVersion(version), partitions, layer_id,
            boost::none, false);

  return result;
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

PORTING_POP_WARNINGS()
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
