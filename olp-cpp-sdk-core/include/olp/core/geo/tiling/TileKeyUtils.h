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

/**
 * @brief Used to get geographic coordinates from tile keys and vise versa.
 */
class CORE_API TileKeyUtils {
 public:
  /**
   * @brief Gets the key of the tile that contains geographic coordinates.
   *
   * @param[in] tiling_scheme The tiling scheme.
   * @param[in] geo_point The geographic coordinates for which
   * you want to find the container tile key.
   * @param[in] level The level of the tile.
   *
   * @return The key of the tile that contains the geographic coordinates.
   * If the tile cannot be calculated, an invalid tile is returned.
   */
  static TileKey GeoCoordinatesToTileKey(const ITilingScheme& tiling_scheme,
                                         const GeoCoordinates& geo_point,
                                         const std::uint32_t level);

  /**
   * @brief Gets the keys of the tile that overlaps with a geographic rectangle.
   *
   * @param[in] tiling_scheme The tiling scheme.
   * @param[in] geo_rectangle The rectangle for which to find overlapping tile keys.
   * @param[in] level The level of the tile key.
   *
   * @return The keys of the tile at the specified level overlapping with the provided
   * geographic rectangle. If the tile cannot be calculated, an empty container is returned.
   */
  static std::vector<TileKey> GeoRectangleToTileKeys(
      const ITilingScheme& tiling_scheme, const GeoRectangle& geo_rectangle,
      const std::uint32_t level);

  /**
   * @brief Gets the tile key that is a relative of a given parent.
   *
   * @param key The tile key to convert to.
   * @param parent_level The level of the parent tile. It is expected to be less than
   * or equal to `key.Level()`.
   *
   * @return The tile key a relative of the parent level.
   */
  static TileKey GetRelativeSubTileKey(const geo::TileKey& key,
                                       std::uint32_t parent_level);

  /**
   * @brief Merges the given parent tile key and the child tile key into an absolute
   * tile key.
   *
   * @param parent The parent tile key.
   * @param sub_tile The child tile key.
   *
   * @return The absolute tile key.
   */
  static TileKey GetAbsoluteSubTileKey(const TileKey& parent,
                                       const TileKey& sub_tile);
};

/**
 * @brief Calculates the space box of a tile.
 *
 * @param[in] tiling_scheme The tiling scheme.
 * @param[in] tile_key The tile key.
 *
 * @return The tile box.
 */
math::AlignedBox3d CORE_API CalculateTileBox(const ITilingScheme& tiling_scheme,
                                             const TileKey& tile_key);

}  // namespace geo
}  // namespace olp
