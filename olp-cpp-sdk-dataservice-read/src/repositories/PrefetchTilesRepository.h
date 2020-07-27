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

#pragma once

#include <map>
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/model/Partitions.h>
#include "PartitionsCacheRepository.h"
#include "generated/model/Index.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

using RootTilesForRequest = std::map<geo::TileKey, uint32_t>;
using SubQuadsResult = std::map<geo::TileKey, std::string>;
using SubQuadsResponse = client::ApiResponse<SubQuadsResult, client::ApiError>;
using SubTilesResult = SubQuadsResult;
using SubTilesResponse = client::ApiResponse<SubTilesResult, client::ApiError>;

class PrefetchTilesRepository {
 public:
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
  static RootTilesForRequest GetSlicedTiles(
      const std::vector<geo::TileKey>& tile_keys, std::uint32_t min,
      std::uint32_t max);

  static SubTilesResponse GetSubTiles(
      const client::HRN& catalog, const std::string& layer_id,
      const PrefetchTilesRequest& request,
      boost::optional<std::int64_t> version,
      const RootTilesForRequest& root_tiles,
      client::CancellationContext context,
      const client::OlpClientSettings& settings);

  static SubQuadsResult FilterSkippedTiles(const PrefetchTilesRequest& request,
                                           bool request_only_input_tiles,
                                           SubQuadsResult sub_tiles);

 protected:
  static SubQuadsResponse GetSubQuads(const client::HRN& catalog,
                                      const std::string& layer_id,
                                      const PrefetchTilesRequest& request,
                                      std::int64_t version, geo::TileKey tile,
                                      int32_t depth,
                                      const client::OlpClientSettings& settings,
                                      client::CancellationContext context);

  static SubQuadsResponse GetVolatileSubQuads(
      const client::HRN& catalog, const std::string& layer_id,
      const PrefetchTilesRequest& request, geo::TileKey tile, int32_t depth,
      const client::OlpClientSettings& settings,
      client::CancellationContext context);

  static void SplitSubtree(RootTilesForRequest& root_tiles_depth,
                           RootTilesForRequest::iterator subtree_to_split,
                           const geo::TileKey& tile_key, std::uint32_t min);
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
