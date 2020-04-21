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

#include "olp/core/geo/tiling/TileKeyUtils.h"

#include <vector>

#include "olp/core/geo/coordinates/GeoCoordinates.h"
#include "olp/core/geo/coordinates/GeoCoordinates3d.h"
#include "olp/core/geo/coordinates/GeoRectangle.h"
#include "olp/core/geo/projection/IProjection.h"
#include "olp/core/geo/tiling/ISubdivisionScheme.h"
#include "olp/core/geo/tiling/ITilingScheme.h"
#include "olp/core/geo/tiling/TileKey.h"
#include "olp/core/math/AlignedBox.h"
#include "olp/core/math/Math.h"

namespace olp {
namespace geo {

TileKey TileKeyUtils::GeoCoordinatesToTileKey(
    const ITilingScheme& tiling_scheme, const GeoCoordinates& geo_point,
    const std::uint32_t level) {
  WorldCoordinates world_point;
  const IProjection& projection = tiling_scheme.GetProjection();
  if (!projection.Project(GeoCoordinates3d(geo_point, 0), world_point))
    return TileKey();

  const ISubdivisionScheme& subdivision_scheme =
      tiling_scheme.GetSubdivisionScheme();
  const auto& level_size = subdivision_scheme.GetLevelSize(level);
  const std::uint32_t cx = level_size.Width();
  const std::uint32_t cy = level_size.Height();

  const WorldAlignedBox world_box =
      tiling_scheme.GetProjection().WorldExtent(0, 0);
  const auto& world_size = world_box.Size();

  const math::Vector3d& world_box_min = world_box.Minimum();
  const math::Vector3d& world_box_max = world_box.Maximum();

  if (world_point.x < world_box_min.x || world_point.x > world_box_max.x ||
      world_point.y < world_box_min.y || world_point.y > world_box_max.y) {
    return TileKey();
  }

  const std::uint32_t column = std::min(
      cx - 1, static_cast<std::uint32_t>(
                  cx * (world_point.x - world_box_min.x) / world_size.x));

  const std::uint32_t row = std::min(
      cy - 1, static_cast<std::uint32_t>(
                  cy * (world_point.y - world_box_min.y) / world_size.y));

  return TileKey::FromRowColumnLevel(row, column, level);
}

std::vector<TileKey> TileKeyUtils::GeoRectangleToTileKeys(
    const ITilingScheme& tiling_scheme, const GeoRectangle& geo_rectangle,
    const std::uint32_t level) {
  if (geo_rectangle.IsEmpty()) {
    return std::vector<TileKey>();
  }

  GeoCoordinates south_west = geo_rectangle.SouthWest();
  GeoCoordinates north_east = geo_rectangle.NorthEast();

  // Clamp at the poles and wrap around the international date line.
  south_west.SetLongitude(
      math::Wrap(south_west.GetLongitude(), -math::pi, math::pi));
  south_west.SetLatitude(
      math::Clamp(south_west.GetLatitude(), -math::half_pi, math::half_pi));

  north_east.SetLongitude(
      math::Wrap(north_east.GetLongitude(), -math::pi, math::pi));
  north_east.SetLatitude(
      math::Clamp(north_east.GetLatitude(), -math::half_pi, math::half_pi));

  const TileKey min_tile_key =
      GeoCoordinatesToTileKey(tiling_scheme, south_west, level);
  const TileKey max_tile_key =
      GeoCoordinatesToTileKey(tiling_scheme, north_east, level);

  const uint32_t column_count =
      tiling_scheme.GetSubdivisionScheme().GetLevelSize(level).Width();

  const uint32_t min_column = min_tile_key.Column();
  uint32_t max_column = max_tile_key.Column();

  // wrap around case
  if (south_west.GetLongitude() > north_east.GetLongitude()) {
    if (max_column != min_column) {
      max_column += column_count;
    } else {
      max_column += column_count - 1;
    }
  }

  std::vector<TileKey> keys;
  for (uint32_t row = min_tile_key.Row(); row <= max_tile_key.Row(); ++row) {
    for (uint32_t column = min_column; column <= max_column; ++column) {
      keys.push_back(
          TileKey::FromRowColumnLevel(row, column % column_count, level));
    }
  }

  return keys;
}

geo::TileKey TileKeyUtils::GetRelativeSubTileKey(const geo::TileKey& key,
                                                 std::uint32_t parent_level) {
  auto origin_key =
      key.ChangedLevelTo(parent_level).ChangedLevelTo(key.Level());
  return geo::TileKey::FromRowColumnLevel(key.Row() - origin_key.Row(),
                                          key.Column() - origin_key.Column(),
                                          key.Level() - parent_level);
}

TileKey TileKeyUtils::GetAbsoluteSubTileKey(const TileKey& parent,
                                            const TileKey& sub_tile) {
  const TileKey abs_sub_tile =
      parent.ChangedLevelBy(static_cast<int>(sub_tile.Level()));
  return TileKey::FromRowColumnLevel(abs_sub_tile.Row() + sub_tile.Row(),
                                     abs_sub_tile.Column() + sub_tile.Column(),
                                     abs_sub_tile.Level());
}

math::AlignedBox3d CalculateTileBox(const ITilingScheme& tiling_scheme,
                                    const TileKey& tile_key) {
  static constexpr double kZeroAlt = 0;
  const auto& subdiv_scheme = tiling_scheme.GetSubdivisionScheme();
  const auto& projection = tiling_scheme.GetProjection();

  const auto world_bounds = projection.WorldExtent(kZeroAlt, kZeroAlt);
  const auto level_size = subdiv_scheme.GetLevelSize(tile_key.Level());

  const double dx = world_bounds.Size().x / level_size.Width();
  const double dy = world_bounds.Size().y / level_size.Height();

  const auto min =
      world_bounds.Minimum() +
      WorldCoordinates{tile_key.Column() * dx, tile_key.Row() * dy, 0};
  const auto max = min + WorldCoordinates{dx, dy, 0};

  return {min, max};
}

}  // namespace geo
}  // namespace olp
