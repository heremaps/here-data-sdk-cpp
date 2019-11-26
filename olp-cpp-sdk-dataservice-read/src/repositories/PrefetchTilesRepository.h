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

using TileKeyAndDepth = std::pair<geo::TileKey, int32_t>;
using SubQuadsRequest = std::map<std::string, TileKeyAndDepth>;
using SubQuadsResult = std::vector<std::pair<geo::TileKey, std::string>>;
using SubQuadsResponse = client::ApiResponse<SubQuadsResult, client::ApiError>;
using SubTilesResult = SubQuadsResult;
using SubTilesResponse = client::ApiResponse<SubTilesResult, client::ApiError>;

class PrefetchTilesRepository final {
 public:
  /**
   * @brief Given tile keys, return all related tile keys that are between
   * minLevel and maxLevel, and the depth. If the tile key is lower than the
   * minimum, the closest decendents are queried instead. If the tile key is
   * higher than the maximum, the closest ancestor are queried instead.
   *
   * @param tilekeys Tile Keys on which to base the search.
   * @param minLevel Minimum level of the resultant tile keys.
   * @param maxLevel Maximum level of the resultant tile keys.
   */
  static SubQuadsRequest EffectiveTileKeys(
      const std::vector<geo::TileKey>& tile_keys, unsigned int min_level,
      unsigned int max_level);

  static SubTilesResponse GetSubTiles(
      const client::HRN& catalog, const std::string& layer_id,
      const PrefetchTilesRequest& request, const SubQuadsRequest& sub_quads,
      client::CancellationContext context,
      const client::OlpClientSettings& settings);

  static SubQuadsResponse GetSubQuads(const client::HRN& catalog,
                                      const std::string& layer_id,
                                      const PrefetchTilesRequest& request,
                                      geo::TileKey tile, int32_t depth,
                                      const client::OlpClientSettings& settings,
                                      client::CancellationContext context);

 private:
  static SubQuadsRequest EffectiveTileKeys(const geo::TileKey& tile_key,
                                           unsigned int min_level,
                                           unsigned int max_level,
                                           bool add_ancestors);

  static std::vector<geo::TileKey> GetChildAtLevel(const geo::TileKey& tile_key,
                                                   unsigned int min_level);
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
