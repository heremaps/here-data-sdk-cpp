/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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

#pragma once

#include <map>
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/ExtendedApiResponse.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/model/Partitions.h>
#include "NamedMutex.h"
#include "PartitionsCacheRepository.h"
#include "generated/model/Index.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

using RootTilesForRequest = std::map<geo::TileKey, uint32_t>;
using SubQuadsResult = std::map<geo::TileKey, std::string>;
using SubQuadsResponse = ExtendedApiResponse<SubQuadsResult, client::ApiError,
                                             client::NetworkStatistics>;
using SubTilesResult = SubQuadsResult;
using SubTilesResponse = ExtendedApiResponse<SubTilesResult, client::ApiError,
                                             client::NetworkStatistics>;

class PrefetchTilesRepository {
 public:
  PrefetchTilesRepository(
      client::HRN catalog, const std::string& layer_id,
      client::OlpClientSettings settings, client::ApiLookupClient client,
      boost::optional<std::string> billing_tag = boost::none,
      NamedMutexStorage mutex_storage = NamedMutexStorage());

  /**
   * @brief Given tile keys, return all related tile keys that are between
   * minLevel and maxLevel, and the depth. This tiles makes possible to cover
   * full requested tree. tile_keys should be root tiles for subtree for
   * request.
   *
   * @param tilekeys Tile Keys on which to base the search.
   * @param minLevel Minimum level of the resultant tile keys.
   * @param maxLevel Maximum level of the resultant tile keys.
   */
  RootTilesForRequest GetSlicedTiles(const std::vector<geo::TileKey>& tile_keys,
                                     std::uint32_t min, std::uint32_t max);

  /**
   * @brief Filters the input tiles according to the request.
   *
   * Removes tiles that do not belong to the minimum and maximum levels.
   * Removes tiles that are not a child or a parent of the requested tiles.
   *
   * @param request Your request.
   * @param tiles The input tiles.
   *
   * @returns The modified tiles.
   */
  SubQuadsResult FilterTilesByLevel(const PrefetchTilesRequest& request,
                                    SubQuadsResult tiles);

  /**
   * @brief Filters the input tiles according to the request.
   *
   * Removes tiles that are not requested.
   * Adds tiles that are missing (to notify you that they are not found).
   * If you requested aggregated tiles, `FilterTilesByList` scans for parents.
   *
   * @param request Your request.
   * @param tiles The input tiles.
   *
   * @returns The modified tiles.
   */
  SubQuadsResult FilterTilesByList(const PrefetchTilesRequest& request,
                                   SubQuadsResult tiles);

  client::NetworkStatistics LoadAggregatedSubQuads(
      geo::TileKey tile, const SubQuadsResult& tiles, std::int64_t version,
      client::CancellationContext context);

  SubQuadsResponse GetVersionedSubQuads(geo::TileKey tile, int32_t depth,
                                        std::int64_t version,
                                        client::CancellationContext context);

  SubQuadsResponse GetVolatileSubQuads(geo::TileKey tile, int32_t depth,
                                       client::CancellationContext context);

 protected:
  static void SplitSubtree(RootTilesForRequest& root_tiles_depth,
                           RootTilesForRequest::iterator subtree_to_split,
                           const geo::TileKey& tile_key, std::uint32_t min);

  using QuadTreeResponse = ExtendedApiResponse<QuadTreeIndex, client::ApiError,
                                               client::NetworkStatistics>;

  QuadTreeResponse DownloadVersionedQuadTree(
      geo::TileKey tile, int32_t depth, std::int64_t version,
      client::CancellationContext context);

 private:
  const client::HRN catalog_;
  const std::string catalog_str_;
  const std::string layer_id_;
  client::OlpClientSettings settings_;
  client::ApiLookupClient lookup_client_;
  PartitionsCacheRepository cache_repository_;
  boost::optional<std::string> billing_tag_;
  NamedMutexStorage storage_;
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
