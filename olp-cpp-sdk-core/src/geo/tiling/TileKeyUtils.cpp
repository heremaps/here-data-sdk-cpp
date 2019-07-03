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

#include <olp/core/geo/tiling/TileKeyUtils.h>

#include <vector>

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/geo/coordinates/GeoCoordinates3d.h>
#include <olp/core/geo/coordinates/GeoRectangle.h>
#include <olp/core/geo/projection/IProjection.h>
#include <olp/core/geo/tiling/ISubdivisionScheme.h>
#include <olp/core/geo/tiling/ITilingScheme.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/math/AlignedBox.h>
#include <olp/core/math/Math.h>

namespace olp {

using namespace math;

namespace geo {
TileKey TileKeyUtils::GeoCoordinatesToTileKey(
    const ITilingScheme& tiling_scheme, const GeoCoordinates& geo_point,
    const std::uint32_t level) {
  WorldCoordinates worldPoint;
  const IProjection& projection = tiling_scheme.GetProjection();
  if (!projection.Project(GeoCoordinates3d(geo_point, 0), worldPoint))
    return TileKey();

  const ISubdivisionScheme& subdivisionScheme =
      tiling_scheme.GetSubdivisionScheme();
  const auto& levelSize = subdivisionScheme.GetLevelSize(level);
  const std::uint32_t cx = levelSize.Width();
  const std::uint32_t cy = levelSize.Height();

  const WorldAlignedBox worldBBox =
      tiling_scheme.GetProjection().WorldExtent(0, 0);
  const auto& worldSize = worldBBox.Size();

  const math::Vector3d& worldBBoxMin = worldBBox.Minimum();
  const math::Vector3d& worldBBoxMax = worldBBox.Maximum();
  if (worldPoint.x < worldBBoxMin.x || worldPoint.x > worldBBoxMax.x)
    return TileKey();

  if (worldPoint.y < worldBBoxMin.y || worldPoint.y > worldBBoxMax.y)
    return TileKey();

  const std::uint32_t column =
      std::min(cx - 1, static_cast<std::uint32_t>(
                           cx * (worldPoint.x - worldBBoxMin.x) / worldSize.x));

  const std::uint32_t row =
      std::min(cy - 1, static_cast<std::uint32_t>(
                           cy * (worldPoint.y - worldBBoxMin.y) / worldSize.y));

  return TileKey::FromRowColumnLevel(row, column, level);
}

std::vector<TileKey> TileKeyUtils::GeoRectangleToTileKeys(
    const ITilingScheme& tiling_scheme, const GeoRectangle& geo_rectangle,
    const std::uint32_t level) {
  if (geo_rectangle.IsEmpty()) return std::vector<TileKey>();

  GeoCoordinates southWest = geo_rectangle.SouthWest();
  GeoCoordinates northEast = geo_rectangle.NorthEast();

  // Clamp at the poles and wrap around the international date line.
  southWest.SetLongitude(
      math::Wrap(southWest.GetLongitude(), -math::pi, math::pi));
  southWest.SetLatitude(
      math::Clamp(southWest.GetLatitude(), -math::half_pi, math::half_pi));

  northEast.SetLongitude(
      math::Wrap(northEast.GetLongitude(), -math::pi, math::pi));
  northEast.SetLatitude(
      math::Clamp(northEast.GetLatitude(), -math::half_pi, math::half_pi));

  const TileKey minTileKey =
      GeoCoordinatesToTileKey(tiling_scheme, southWest, level);
  const TileKey maxTileKey =
      GeoCoordinatesToTileKey(tiling_scheme, northEast, level);

  const uint32_t columnCount =
      tiling_scheme.GetSubdivisionScheme().GetLevelSize(level).Width();

  const uint32_t minColumn = minTileKey.Column();
  uint32_t maxColumn = maxTileKey.Column();

  // wrap around case
  if (southWest.GetLongitude() > northEast.GetLongitude()) {
    if (maxColumn != minColumn)
      maxColumn += columnCount;
    else  // do not duplicate
      maxColumn += columnCount - 1;
  }

  std::vector<TileKey> keys;
  for (uint32_t row = minTileKey.Row(); row <= maxTileKey.Row(); ++row)
    for (uint32_t column = minColumn; column <= maxColumn; ++column)
      keys.push_back(
          TileKey::FromRowColumnLevel(row, column % columnCount, level));

  return keys;
}

geo::TileKey TileKeyUtils::GetRelativeSubTileKey(const geo::TileKey& key,
                                                 std::uint32_t parent_level) {
  auto originKey = key.ChangedLevelTo(parent_level).ChangedLevelTo(key.Level());
  return geo::TileKey::FromRowColumnLevel(key.Row() - originKey.Row(),
                                          key.Column() - originKey.Column(),
                                          key.Level() - parent_level);
}

TileKey TileKeyUtils::GetAbsoluteSubTileKey(const TileKey& parent,
                                            const TileKey& sub_tile) {
  const TileKey absSubTileKey =
      parent.ChangedLevelBy(static_cast<int>(sub_tile.Level()));
  return TileKey::FromRowColumnLevel(absSubTileKey.Row() + sub_tile.Row(),
                                     absSubTileKey.Column() + sub_tile.Column(),
                                     absSubTileKey.Level());
}

AlignedBox3d calculateTileBox(const ITilingScheme& tiling_scheme,
                              const TileKey& tile_key) {
  static const double ZERO_ALT = 0;
  const auto& subdivScheme = tiling_scheme.GetSubdivisionScheme();
  const auto& projection = tiling_scheme.GetProjection();

  const auto worldBounds = projection.WorldExtent(ZERO_ALT, ZERO_ALT);
  const auto levelSize = subdivScheme.GetLevelSize(tile_key.Level());

  const double dx = worldBounds.Size().x / levelSize.Width();
  const double dy = worldBounds.Size().y / levelSize.Height();

  const auto min = worldBounds.Minimum() +
                   Vector3d{tile_key.Column() * dx, tile_key.Row() * dy, 0};
  const auto max = min + Vector3d{dx, dy, 0};

  return {min, max};
}

}  // namespace geo
}  // namespace olp
