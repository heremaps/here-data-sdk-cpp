/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/Data.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Represents the result of a aggregated data operation.
 */
struct DATASERVICE_READ_API AggregatedDataResult {
  /**
   * @brief The `TileKey` instance that addresses a tile in a quadtree.
   *
   * @param tile_key The fetched tile key.
   */
  geo::TileKey tile_key;

  /**
   * @brief The data of the tile.
   *
   * @param data The prefetched data of the tile.
   */
  model::Data data;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
