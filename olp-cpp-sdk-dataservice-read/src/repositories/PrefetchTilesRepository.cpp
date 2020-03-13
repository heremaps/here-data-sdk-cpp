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
#include <vector>

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/Atomic.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/TileRequest.h>
#include "ApiClientLookup.h"
#include "PartitionsRepository.h"
#include "generated/api/QueryApi.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "PrefetchTilesRepository";
constexpr std::uint32_t kMaxQuadTreeIndexDepth = 4u;
}  // namespace

using namespace olp::client;

SubQuadsRequest PrefetchTilesRepository::EffectiveTileKeys(
    const std::vector<geo::TileKey>& tile_keys, unsigned int min_level,
    unsigned int max_level) {
  SubQuadsRequest ret;
  for (auto tile_key : tile_keys) {
    auto child_tiles = EffectiveTileKeys(tile_key, min_level, max_level, true);
    for (auto tile : child_tiles) {
      // check if child already exist, if so, use the greater depth
        //some tiles could be requested twice, as they can be childrens with depth less than 4
      auto old_child = ret.find(tile.first);
      if (old_child != ret.end()) {
        ret[tile.first] = std::max((*old_child).second, tile.second);
      } else {
        ret[tile.first] = tile.second;
      }
    }
  }

  return ret;
}

SubQuadsRequest PrefetchTilesRepository::EffectiveTileKeys(
    const geo::TileKey& tile_key, unsigned int min_level,
    unsigned int max_level, bool add_ancestors) {
  SubQuadsRequest ret;
  auto current_level = tile_key.Level();

  auto FindParentAtLevel = [&](unsigned int steps,
                               unsigned int depth_after_current)
      -> std::pair<std::string, TileKeyAndDepth> {
    auto tile{tile_key};
    unsigned int steps_left = steps;
    while (steps_left > 0 && tile.IsValid()) {
      tile = tile.Parent();
      --steps_left;
    }
    return std::make_pair(
        tile.ToHereTile(),
        std::make_pair(tile,
                       std::min(kMaxQuadTreeIndexDepth,
                                steps - steps_left + depth_after_current)));
  };

  // min/max values was not set, or wrong. Use default wide range.
  if ((max_level == 0 && min_level == 0) || (min_level > max_level) ||
      (max_level >= geo::TileKey().Level())) {
    min_level = 1;
    max_level = geo::TileKey().Level();
  }
  // ignore some tiles from min/max range, if range of min/max wider than 4. Use
  // parrent tile which will include requested tile

  if (current_level > max_level) {
    // if this tile is greater than max, find the parent of this(go only for
    // depth 4) and push it
    ret.emplace(FindParentAtLevel(
        std::min(kMaxQuadTreeIndexDepth, current_level - min_level), 0));

  } else if (current_level < min_level) {
    // if this tile is less than min, just add this tile with depth 4
    ret.emplace(std::make_pair(
        tile_key.ToHereTile(),
        std::make_pair(tile_key, std::min(kMaxQuadTreeIndexDepth,
                                          max_level - current_level))));
  } else {
    // if min/max range less than 4
    if (max_level - min_level <= kMaxQuadTreeIndexDepth) {
      // just add parent with depth
      ret.emplace(FindParentAtLevel(current_level - min_level,
                                    max_level - current_level));
    } else {
      // if min/max range greter than 4, just go up till min if possible, or go 4 levels up, and add tile with
      // depth 4
      ret.emplace(FindParentAtLevel(
          std::min(kMaxQuadTreeIndexDepth, current_level - min_level),
          geo::TileKey().Level()));  // to add max depth use 32
    }
  }
  return ret;
}

std::vector<geo::TileKey> PrefetchTilesRepository::GetChildAtLevel(
    const geo::TileKey& tile_key, unsigned int min_level) {
  if (!tile_key.IsValid()) {
    return {};
  }
  if (tile_key.Level() >= min_level) {
    return {tile_key};
  }

  std::vector<geo::TileKey> ret;
  for (std::uint8_t index = 0; index < kMaxQuadTreeIndexDepth; ++index) {
    auto child = GetChildAtLevel(tile_key.GetChild(index), min_level);
    ret.insert(ret.end(), child.begin(), child.end());
  }

  return ret;
}

SubQuadsResponse PrefetchTilesRepository::GetSubTiles(
    const client::HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, const SubQuadsRequest& sub_quads,
    CancellationContext context, const client::OlpClientSettings& settings) {
  // Version needs to be set, else we cannot move forward
  if (!request.GetVersion()) {
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "GetSubTiles: catalog version missing, key=%s",
                          request.CreateKey(layer_id).c_str());
    return ApiError{ErrorCode::InvalidArgument, "Catalog version invalid"};
  }
  SubQuadsResult result;
  OLP_SDK_LOG_INFO_F(kLogTag, "GetSubTiles: hrn=%s, layer=%s, quads=%zu",
                     catalog.ToString().c_str(), layer_id.c_str(),
                     sub_quads.size());

  for (const auto& quad : sub_quads) {
    if (context.IsCancelled()) {
      return client::ApiError{ErrorCode::Cancelled, "Cancelled", true};
    }

    auto& tile = quad.second.first;
    auto& depth = quad.second.second;

    auto response =
        GetSubQuads(catalog, layer_id, request, tile, depth, settings, context);

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

TilesResult PrefetchTilesRepository::GetSubQuadsFromCache(
    const client::HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, client::CancellationContext context,
    int64_t version, const client::OlpClientSettings& settings) {
  TilesResult result;
  if (!context.IsCancelled()) {
    for (const auto& tile : request.GetTileKeys()) {
      model::Partitions rez = PartitionsRepository::GetTileFromCache(
          catalog, layer_id,
          TileRequest().WithTileKey(tile).WithBillingTag(
              request.GetBillingTag()),
          version, settings);
      if (rez.GetPartitions().empty()) {
        result.tile_keys.push_back(tile);
      } else {
        result.found_tiles[tile] = rez.GetPartitions().front().GetDataHandle();
      }
    }
  }
  return result;
}

SubQuadsResponse PrefetchTilesRepository::GetSubQuads(
    const HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, geo::TileKey tile, int32_t depth,
    const OlpClientSettings& settings, CancellationContext context) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetSubQuads(%s, %" PRId64 ", %" PRId32 ")",
                      tile.ToHereTile().c_str(), request.GetVersion().get(),
                      depth);

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
  auto version = request.GetVersion().get();

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "GetSubQuads execute(%s, %" PRId64 ", %" PRId32 ")",
                     tile_key.c_str(), version, depth);
  auto quad_tree = QueryApi::QuadTreeIndex(
      query_api.GetResult(), layer_id, version, tile_key, depth, boost::none,
      request.GetBillingTag(), context);

  if (!quad_tree.IsSuccessful()) {
    OLP_SDK_LOG_INFO_F(kLogTag,
                       "GetSubQuads failed(%s, %" PRId64 ", %" PRId32 ")",
                       tile_key.c_str(), version, depth);
    return quad_tree.GetError();
  }

  SubQuadsResult result;
  model::Partitions partitions;

  const auto& subquads = quad_tree.GetResult().GetSubQuads();
  partitions.GetMutablePartitions().reserve(subquads.size());

  for (const auto& subquad : subquads) {
    auto subtile = tile.AddedSubHereTile(subquad->GetSubQuadKey());

    // Add to result
    result.emplace(subtile, subquad->GetDataHandle());

    // add to bulk partitions for cacheing
    partitions.GetMutablePartitions().emplace_back(
        PartitionsRepository::PartitionFromSubQuad(*subquad, subtile.ToHereTile()));
  }

  // add to cache
  repository::PartitionsCacheRepository cache(catalog, settings.cache);
  cache.Put(PartitionsRequest().WithVersion(version), partitions, layer_id,
            boost::none, false);

  return result;
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
