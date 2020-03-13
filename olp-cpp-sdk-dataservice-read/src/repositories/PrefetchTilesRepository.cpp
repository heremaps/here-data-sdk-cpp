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

using namespace olp::client;

SubQuadsRequest PrefetchTilesRepository::EffectiveTileKeys(
    const std::vector<geo::TileKey>& tile_keys, unsigned int min_level,
    unsigned int max_level) {
  SubQuadsRequest ret;
  for (auto tile_key : tile_keys) {
    auto child_tiles = EffectiveTileKeys(tile_key, min_level, max_level, true);
    for (auto tile : child_tiles) {
      // check if child already exist, if so, use the greater depth
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

  // min/max values was not set. Use default wide range.
  if (max_level == 0 && min_level == 0) {
    max_level = geo::TileKey().Level();
  }

  auto AddParents = [&]() {
    auto tile{tile_key.Parent()};
    while (tile.Level() >= min_level && tile.IsValid()) {
      if (tile.Level() <= max_level)
        ret[tile.ToHereTile()] = std::make_pair(tile, 0);
      tile = tile.Parent();
    }
  };

  auto AddChildren = [&](const geo::TileKey& tile, unsigned int min,
                         unsigned int max) {
    auto child_tiles = EffectiveTileKeys(tile, min, max, false);
    for (auto tile : child_tiles) {
      // check if child already exist, if so, use the greater depth
      auto old_child = ret.find(tile.first);
      if (old_child != ret.end())
        ret[tile.first] = std::max((*old_child).second, tile.second);
      else
        ret[tile.first] = tile.second;
    }
  };

  if (current_level > max_level) {
    // if this tile is greater than max, find the parent of this and push the
    // ones between min and max
    AddParents();
  } else if (current_level < min_level) {
    // if this tile is less than min, find the children that are min and start
    // from there
    auto children = GetChildAtLevel(tile_key, min_level);
    for (auto child : children) {
      AddChildren(child, min_level, max_level);
    }
  } else {
    // tile is within min and max
    if (add_ancestors) {
      AddParents();
    }

    auto tile_key_str = tile_key.ToHereTile();
    if (max_level - current_level <= kMaxQuadTreeIndexDepth) {
      ret[tile_key_str] = std::make_pair(tile_key, max_level - current_level);
    } else {
      // Backend only takes kMaxQuadTreeIndexDepth at a time, so we have to
      // manually calculate all the tiles that should included
      ret[tile_key_str] = std::make_pair(tile_key, kMaxQuadTreeIndexDepth);
      auto children =
          GetChildAtLevel(tile_key, current_level + kMaxQuadTreeIndexDepth + 1);
      for (auto child : children) {
        AddChildren(child, current_level,
                    current_level + kMaxQuadTreeIndexDepth);
      }
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

SubTilesResponse PrefetchTilesRepository::GetSubTiles(
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
  SubTilesResult result;
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
    result.reserve(result.size() + subtiles.size());
    result.insert(result.end(), std::make_move_iterator(subtiles.begin()),
                  std::make_move_iterator(subtiles.end()));
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
  result.reserve(subquads.size());
  partitions.GetMutablePartitions().reserve(subquads.size());

  for (const auto& subquad : subquads) {
    auto subtile = tile.AddedSubHereTile(subquad->GetSubQuadKey());

    // Add to result
    result.emplace_back(subtile, subquad->GetDataHandle());

    // add to bulk partitions for cacheing
    partitions.GetMutablePartitions().emplace_back(
        PartitionsRepository::PartitionFromSubQuad(*subquad,
                                                   subtile.ToHereTile()));
  }

  // add to cache
  repository::PartitionsCacheRepository cache(catalog, settings.cache);
  cache.Put(PartitionsRequest().WithVersion(version), partitions, layer_id,
            boost::none, false);

  return result;
}

PORTING_POP_WARNINGS()
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
