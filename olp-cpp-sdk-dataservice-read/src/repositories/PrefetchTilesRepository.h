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

#include <olp/core/client/HRN.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/model/Partitions.h>

#include "ApiRepository.h"
#include "PartitionsCacheRepository.h"
#include "generated/model/Index.h"
#include "olp/dataservice/read/PrefetchTilesRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

using TileKeyAndDepth = std::pair<geo::TileKey, int32_t>;
using SubQuadsRequest = std::map<std::string, TileKeyAndDepth>;
using SubQuadsResult = std::vector<std::pair<geo::TileKey, std::string>>;
using SubQuadsResponse = client::ApiResponse<SubQuadsResult, client::ApiError>;
using SubQuadsResponseCallback =
    std::function<void(const SubQuadsResponse& response)>;

using SubTilesResult = SubQuadsResult;
using SubTilesResponse = client::ApiResponse<SubTilesResult, client::ApiError>;
using SubTilesResponseCallback =
    std::function<void(const SubTilesResponse& response)>;

class PrefetchTilesRepository final {
 public:
  PrefetchTilesRepository(
      const client::HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
      std::shared_ptr<PartitionsCacheRepository> partitionsCache,
      std::shared_ptr<olp::client::OlpClientSettings> settings);

  ~PrefetchTilesRepository() = default;

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
      const std::vector<geo::TileKey>& tilekeys, unsigned int minLevel,
      unsigned int maxLevel);

  void GetSubTiles(
      std::shared_ptr<client::CancellationContext> cancellationContext,
      const PrefetchTilesRequest& prefetchRequest, int64_t version,
      boost::optional<time_t> expiry, const SubQuadsRequest& request,
      const SubTilesResponseCallback& callback);

  static void GetSubQuads(
      std::shared_ptr<client::CancellationContext> cancellationContext,
      std::shared_ptr<ApiRepository> apiRepo,
      PartitionsCacheRepository& partitionsCache,
      const PrefetchTilesRequest& prefetchRequest, geo::TileKey tile,
      int64_t version, boost::optional<time_t> expiry, int32_t depth,
      const SubQuadsResponseCallback& callback);

 private:
  static SubQuadsRequest EffectiveTileKeys(const geo::TileKey& tilekey,
                                           unsigned int minLevel,
                                           unsigned int maxLevel,
                                           bool addAncestors);

  static std::vector<geo::TileKey> GetChildAtLevel(const geo::TileKey& tilekey,
                                                   unsigned int minLevel);

  static model::Partition PartitionFromSubQuad(
      std::shared_ptr<model::SubQuad> subQuad, const std::string& partition);

 private:
  client::HRN hrn_;
  std::shared_ptr<ApiRepository> apiRepo_;
  std::shared_ptr<PartitionsCacheRepository> partitionsCache_;
  std::shared_ptr<olp::client::OlpClientSettings> settings_;
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
