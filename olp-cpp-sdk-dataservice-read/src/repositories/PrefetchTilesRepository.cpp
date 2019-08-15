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

#include "ExecuteOrSchedule.inl"
#include "PrefetchTilesRepository.h"

#include <olp/core/logging/Log.h>

#include <olp/core/geo/tiling/TileKey.h>
#include "generated/api/QueryApi.h"

#include <inttypes.h>
#include <vector>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "PrefetchTilesRepository";
constexpr std::uint32_t kMaxQuadTreeIndexDepth = 4u;
}  // namespace

using namespace olp::client;

PrefetchTilesRepository::PrefetchTilesRepository(
    const HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
    std::shared_ptr<PartitionsCacheRepository> partitionsCache)
    : hrn_(hrn), apiRepo_(apiRepo), partitionsCache_(partitionsCache) {}

SubQuadsRequest PrefetchTilesRepository::EffectiveTileKeys(
    const std::vector<geo::TileKey>& tilekeys, unsigned int minLevel,
    unsigned int maxLevel) {
  SubQuadsRequest ret;
  for (auto tilekey : tilekeys) {
    auto childTiles = EffectiveTileKeys(tilekey, minLevel, maxLevel, true);
    for (auto tile : childTiles) {
      // check if child already exist, if so, use the greater depth
      auto oldChild = ret.find(tile.first);
      if (oldChild != ret.end()) {
        ret[tile.first] = std::max((*oldChild).second, tile.second);
      } else {
        ret[tile.first] = tile.second;
      }
    }
  }

  return ret;
}

SubQuadsRequest PrefetchTilesRepository::EffectiveTileKeys(
    const geo::TileKey& tilekey, unsigned int minLevel, unsigned int maxLevel,
    bool addAncestors) {
  SubQuadsRequest ret;
  auto currentLevel = tilekey.Level();

  auto AddParents = [tilekey, minLevel, maxLevel, &ret]() {
    auto tile{tilekey.Parent()};
    while (tile.Level() >= minLevel) {
      if (tile.Level() <= maxLevel)
        ret[tile.ToHereTile()] = std::make_pair(tile, 0);
      tile = tile.Parent();
    }
  };

  auto AddChildren = [minLevel, maxLevel, &ret](const geo::TileKey& tilekey) {
    auto childTiles = EffectiveTileKeys(tilekey, minLevel, maxLevel, false);
    for (auto tile : childTiles) {
      // check if child already exist, if so, use the greater depth
      auto oldChild = ret.find(tile.first);
      if (oldChild != ret.end())
        ret[tile.first] = std::max((*oldChild).second, tile.second);
      else
        ret[tile.first] = tile.second;
    }
  };

  if (currentLevel > maxLevel) {
    // if this tile is greater than max, find the parent of this and push the
    // ones between min and max
    AddParents();
  } else if (currentLevel < minLevel) {
    // if this tile is less than min, find the children that are min and start
    // from there
    auto children = GetChildAtLevel(tilekey, minLevel);
    for (auto child : children) AddChildren(child);
  } else {
    // tile is within min and max
    if (addAncestors) AddParents();

    auto tilekeyStr = tilekey.ToHereTile();
    if (maxLevel - currentLevel <= kMaxQuadTreeIndexDepth)
      ret[tilekeyStr] = std::make_pair(tilekey, maxLevel - currentLevel);
    else {
      // Backend only takes MAX_QUAD_TREE_INDEX_DEPTH at a time, so we have to
      // manually calcuate all the tiles that should included
      ret[tilekeyStr] = std::make_pair(tilekey, kMaxQuadTreeIndexDepth);
      auto children =
          GetChildAtLevel(tilekey, currentLevel + kMaxQuadTreeIndexDepth + 1);
      for (auto child : children) AddChildren(child);
    }
  }

  return ret;
}

std::vector<geo::TileKey> PrefetchTilesRepository::GetChildAtLevel(
    const geo::TileKey& tilekey, unsigned int minLevel) {
  std::vector<geo::TileKey> ret;
  if (tilekey.Level() >= minLevel) return {tilekey};

  for (std::uint8_t i = 0; i < 4; i++) {
    auto child = GetChildAtLevel(tilekey.GetChild(i), minLevel);
    ret.insert(ret.end(), child.begin(), child.end());
  }

  return ret;
}

void PrefetchTilesRepository::GetSubTiles(
    std::shared_ptr<client::CancellationContext> cancel_context,
    const PrefetchTilesRequest& prefetchRequest, int64_t version,
    boost::optional<time_t> expiry, const SubQuadsRequest& request,
    const SubTilesResponseCallback& callback) {
  auto api_repo = apiRepo_;
  auto& partitions_cache = *partitionsCache_;

  std::thread([=, &partitions_cache]() {
    std::vector<std::future<SubQuadsResponse>> futures;

    for (auto subtile : request) {
      futures.push_back(std::async([=, &partitions_cache]() {
        auto p = std::make_shared<std::promise<SubQuadsResponse>>();
        GetSubQuads(
            cancel_context, api_repo, partitions_cache, prefetchRequest,
            subtile.second.first, version, expiry, subtile.second.second,
            [=](const SubQuadsResponse& response) { p->set_value(response); });

        return p->get_future().get();
      }));
    }

    auto results = std::make_shared<SubTilesResult>();
    for (auto& f : futures) {
      auto response = f.get();
      if (response.IsSuccessful()) {
        for (auto subresult : response.GetResult()) {
          results->push_back(subresult);
        }
      } else {
        // TODO do something with the error
      }
    }

    callback({*results});
  }).detach();
}

void PrefetchTilesRepository::GetSubQuads(
    std::shared_ptr<client::CancellationContext> cancel_context,
    std::shared_ptr<ApiRepository> apiRepo,
    PartitionsCacheRepository& partitionsCache,
    const PrefetchTilesRequest& prefetchRequest, geo::TileKey tile,
    int64_t version, boost::optional<time_t> expiry, int32_t depth,
    const SubQuadsResponseCallback& callback) {
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetSubQuads(%s, %" PRId64 ", %" PRId32 ")",
                       tile.ToHereTile().c_str(), version, depth);

  auto cancel_callback = [callback, tile, version, depth]() {
    EDGE_SDK_LOG_INFO_F(kLogTag,
                        "GetSubQuads cancelled(%s, %" PRId64 ", %" PRId32 ")",
                        tile.ToHereTile().c_str(), version, depth);
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  auto tile_key = tile.ToHereTile();

  cancel_context->ExecuteOrCancelled(
      [=, &partitionsCache]() {
        return apiRepo->getApiClient(
            "query", "v1", [=, &partitionsCache](ApiClientResponse response) {
              if (!response.IsSuccessful()) {
                callback(response.GetError());
                return;
              }

              cancel_context->ExecuteOrCancelled(
                  [=, &partitionsCache]() {
                    EDGE_SDK_LOG_INFO_F(
                        kLogTag,
                        "QuadTreeIndex execute(%s, %" PRId64 ", %" PRId32 ")",
                        tile.ToHereTile().c_str(), version, depth);
                    return QueryApi::QuadTreeIndex(
                        response.GetResult(), prefetchRequest.GetLayerId(),
                        version, tile_key, depth, boost::none,
                        prefetchRequest.GetBillingTag(),
                        [=, &partitionsCache](
                            QueryApi::QuadTreeIndexResponse indexResponse) {
                          if (!indexResponse.IsSuccessful()) {
                            EDGE_SDK_LOG_INFO_F(
                                kLogTag,
                                "QuadTreeIndex Error(%s, %" PRId64 ", %" PRId32
                                ")",
                                tile.ToHereTile().c_str(), version, depth);
                            callback(indexResponse.GetError());
                            return;
                          }

                          EDGE_SDK_LOG_INFO(kLogTag, "QuadTreeIndex Success");
                          SubQuadsResult result;
                          model::Partitions partitions;
                          for (auto subquad :
                               indexResponse.GetResult().GetSubQuads()) {
                            auto subtile =
                                tile.AddedSubHereTile(subquad->GetSubQuadKey());

                            std::pair<geo::TileKey, std::string> p =
                                std::make_pair(subtile,
                                               subquad->GetDataHandle());

                            // add to bulk partitions for cacheing
                            partitions.GetMutablePartitions().push_back(
                                PartitionFromSubQuad(subquad,
                                                     subtile.ToHereTile()));
                            result.push_back(p);
                          }

                          // add to cache
                          PartitionsRequest mockPartitionsRequest;
                          mockPartitionsRequest
                              .WithLayerId(prefetchRequest.GetLayerId())
                              .WithVersion(version);
                          partitionsCache.Put(mockPartitionsRequest, partitions,
                                              expiry,
                                              false);  // get layer expiry

                          callback(result);
                        });
                  },
                  cancel_callback);
            });
      },
      cancel_callback);
}

model::Partition PrefetchTilesRepository::PartitionFromSubQuad(
    std::shared_ptr<model::SubQuad> subQuad, const std::string& partition) {
  model::Partition ret;
  ret.SetPartition(partition);
  ret.SetDataHandle(subQuad->GetDataHandle());
  ret.SetVersion(subQuad->GetVersion());

  ret.SetDataSize(subQuad->GetDataSize());
  ret.SetChecksum(subQuad->GetChecksum());
  ret.SetCompressedDataSize(subQuad->GetCompressedDataSize());

  return ret;
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
