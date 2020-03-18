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

std::vector<geo::TileKey> PrefetchTilesRepository::GetChilds(
    const geo::TileKey& tile_key) {
  if (!tile_key.IsValid()) {
    return {};
  }
  std::vector<geo::TileKey> ret;
  for (std::uint8_t index = 0; index < kMaxQuadTreeIndexDepth; ++index) {
    auto tile = tile_key.GetChild(index);
    if (tile_key.IsValid()) {
      ret.emplace_back(std::move(tile));
    }
  }
  return ret;
}

std::vector<geo::TileKey> PrefetchTilesRepository::GetChildsAt4LevelsDown(
    const geo::TileKey& tile_key) {
  int levels = kMaxQuadTreeIndexDepth + 1;
  std::vector<geo::TileKey> childrens;
  std::vector<geo::TileKey> tiles{tile_key};
  childrens.reserve(std::pow(4, 4));
  tiles.reserve(std::pow(4, 4));

  while (levels-- > 0) {
    for (const auto& child : tiles) {
      auto tile_childrens = GetChilds(child);
      childrens.insert(childrens.end(),
                       std::make_move_iterator(tile_childrens.begin()),
                       std::make_move_iterator(tile_childrens.end()));
    }
    if (childrens.empty()) {
      break;
    }
    tiles = childrens;
    childrens.clear();
  }
  return tiles;
}

void PrefetchTilesRepository::SplitSubtree(
    RootTilesForRequest& root_tiles_depth,
    RootTilesForRequest::iterator subtree_to_split) {
  unsigned int depth = subtree_to_split->second;
  if (depth <= kMaxQuadTreeIndexDepth) {
    return;
  }

  std::vector<geo::TileKey>
      childrens_to_process;  // total child tiles to process
  childrens_to_process.reserve(std::pow(4, depth));
  std::vector<geo::TileKey> sliced_children;
  std::vector<geo::TileKey> next_slice = GetChildsAt4LevelsDown(
      subtree_to_split->first);  // first slice of children

  while (depth > kMaxQuadTreeIndexDepth) {
    sliced_children.swap(next_slice);
    next_slice.clear();
    // get all childrens 4 levels down for each slice
    for (const auto& tile : sliced_children) {
      auto children = GetChildsAt4LevelsDown(tile);
      // add all children's of given tile to next slice
      next_slice.insert(next_slice.end(),
                        std::make_move_iterator(children.begin()),
                        std::make_move_iterator(children.end()));
    }
    // add slice to childrens to process
    childrens_to_process.insert(
        childrens_to_process.end(),
        std::make_move_iterator(sliced_children.begin()),
        std::make_move_iterator(sliced_children.end()));

    depth -= (kMaxQuadTreeIndexDepth +
              1);  // max depth is 4, but root child is on depth 0
  }

  for (const auto& tile : childrens_to_process) {
    auto it = root_tiles_depth.insert({tile, 4});
    // element already exist, update depth with max value(should never hapen)
    if (it.second == false) {
      it.first->second = 4;
    }
  }

}

RootTilesForRequest PrefetchTilesRepository::GetSlicedTilesForRequest(
    const std::vector<geo::TileKey>& tile_keys, unsigned int min,
    unsigned int max) {
  // adjust min/max values.
  if (min > max || max >= geo::TileKey().Level()) {
    // min/max levels are wrong. Reset to dafault case, which is 0/0
    min = max = 0;
  }

  RootTilesForRequest root_tiles_depth;
  // adjust root tiles to min level
  for (auto tile_key : tile_keys) {
    unsigned int min_level = min;
    unsigned int max_level = max;

    if (max_level == min_level) {
      if (max_level == 0) {  // special case, adjust levels to input tile level
        max_level = tile_key.Level();
        min_level =
            (max_level > kMaxQuadTreeIndexDepth) ? (max_level - 4) : (1);
      } else {
        // min max values are the same
        // to reduce methadata requests, go 4 levels up
        min_level =
            (min_level > kMaxQuadTreeIndexDepth) ? (min_level - 4) : (1);
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
          it.first->second = kMaxQuadTreeIndexDepth;
        }
      }
      tile_key = parent;
    }
  }
  return root_tiles_depth;
}

SubTilesResponse PrefetchTilesRepository::GetSubTiles(
    const client::HRN& catalog, const std::string& layer_id,
    const PrefetchTilesRequest& request, const RootTilesForRequest& root_tiles,
    client::CancellationContext context,
    const client::OlpClientSettings& settings) {
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
                     root_tiles.size());

  for (const auto& quad : root_tiles) {
    if (context.IsCancelled()) {
      return client::ApiError{ErrorCode::Cancelled, "Cancelled", true};
    }

    auto& tile = quad.first;
    auto& depth = quad.second;
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
  // result.reserve(subquads.size());
  partitions.GetMutablePartitions().reserve(subquads.size());

  for (const auto& subquad : subquads) {
    auto subtile = tile.AddedSubHereTile(subquad->GetSubQuadKey());
    OLP_SDK_LOG_INFO_F(kLogTag, "GetSubQuad key(%s, %" PRId64 ", %" PRId32 ")",
                       subtile.ToHereTile().c_str(), version, depth);
    // Add to result
    result.emplace(subtile, subquad->GetDataHandle());

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
