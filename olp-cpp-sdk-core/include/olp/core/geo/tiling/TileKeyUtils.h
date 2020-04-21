/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <cstdint>
#include <vector>

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/geo/tiling/TileKey.h>

namespace olp {
namespace geo {

class CORE_API TileKeyUtils {
 public:
  /**
   * @brief Get a TileKey containing specified geo point.
   *
   * @param[in] tiling_scheme Tiling scheme.
   * @param[in] geo_point Geo coordinates to find container tile key for.
   * @param[in] level Level of the tile key.
   *
   * @return Tile key at specified level containing geoPoint. When tile can not
   * be calculated invalid tile is returned.
   */
  static TileKey GeoCoordinatesToTileKey(const ITilingScheme& tiling_scheme,
                                         const GeoCoordinates& geo_point,
                                         const std::uint32_t level);

  /**
   * @brief Get a tile keys overlapping with specified geo rectangle.
   *
   * @param[in] tiling_scheme Tiling scheme.
   * @param[in] geo_rectangle rectangle to find overlapping tile keys for.
   * @param[in] level Level of the tile key.
   *
   * @return Tile keys at specified level overlapping with provided geo
   * rectangle. When tile can not be calculated empty container is returned.
   */
  static std::vector<TileKey> GeoRectangleToTileKeys(
      const ITilingScheme& tiling_scheme, const GeoRectangle& geo_rectangle,
      const std::uint32_t level);

  /**
   * @brief Returns the tile key relative to a given parent.
   *
   * @param key The tile key to convert.
   * @param parent_level The parent level. Parent level is expected to be less
   * or equal to key.Level().
   *
   * @return Tile key relative to the parent level.
   */
  static TileKey GetRelativeSubTileKey(const geo::TileKey& key,
                                       std::uint32_t parent_level);

  /**
   * @brief Merges the given parent tile key and the sub tile key to an absolute
   * tile key.
   *
   * @param parent The parent tile key.
   * @param sub_tile The sub tile key.
   *
   * @return The absolute tile key.
   */
  static TileKey GetAbsoluteSubTileKey(const TileKey& parent,
                                       const TileKey& sub_tile);
};

/**
 * @brief Calculate tiling-space box of a tile.
 *
 * @param[in] tiling_scheme The tiling scheme used.
 * @param[in] tile_key Tile key.
 *
 * @return Tile box
 */
math::AlignedBox3d CORE_API CalculateTileBox(const ITilingScheme& tiling_scheme,
                                             const TileKey& tile_key);

}  // namespace geo
}  // namespace olp
