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

#include "olp/core/geo/projection/SphereProjection.h"

#include "olp/core/geo/Types.h"
#include "olp/core/geo/coordinates/GeoCoordinates.h"
#include "olp/core/geo/coordinates/GeoCoordinates3d.h"
#include "olp/core/geo/coordinates/GeoRectangle.h"
#include "olp/core/geo/projection/EarthConstants.h"
#include "olp/core/math/AlignedBox.h"
#include "olp/core/math/Math.h"

namespace olp {
namespace geo {
namespace {
static WorldCoordinates WorldNormal(const GeoCoordinates& geo_coordinates) {
  const double cos_latitude = cos(geo_coordinates.GetLatitude());
  return {cos(geo_coordinates.GetLongitude()) * cos_latitude,
          sin(geo_coordinates.GetLongitude()) * cos_latitude,
          sin(geo_coordinates.GetLatitude())};
}

WorldCoordinates ToWorldCoordinates(const GeoCoordinates3d& geo_coordinates) {
  const double radius =
      EarthConstants::EquatorialRadius() + geo_coordinates.GetAltitude();
  return WorldNormal(geo_coordinates.GetGeoCoordinates()) * radius;
}

GeoCoordinates3d ToGeoCoordinates(const WorldCoordinates& point) {
  const double parallel_radius_square = point.x * point.x + point.y * point.y;
  const double parallel_radius = sqrt(parallel_radius_square);

  // World-space origin maps to any lat, lng
  const double v = point.z / parallel_radius;
  if (math::isnan(v)) {
    return {0, 0, -EarthConstants::EquatorialRadius()};
  }

  const double radius = sqrt(parallel_radius_square + point.z * point.z);

  return {atan(v), atan2(point.y, point.x),
          radius - EarthConstants::EquatorialRadius()};
}

}  // namespace

GeoRectangle SphereProjection::GetGeoBounds() const {
  return {{-math::half_pi, -math::pi}, {+math::half_pi, +math::pi}};
}

WorldAlignedBox SphereProjection::WorldExtent(double,
                                              double maximum_altitude) const {
  auto radius = WorldCoordinates(1, 1, 1) *
                (EarthConstants::EquatorialRadius() + maximum_altitude);
  return WorldAlignedBox(-radius, radius);
}

bool SphereProjection::Project(const GeoCoordinates3d& geo_point,
                               WorldCoordinates& world_point) const {
  world_point = ToWorldCoordinates(geo_point);
  return true;
}

bool SphereProjection::Unproject(const WorldCoordinates& world_point,
                                 GeoCoordinates3d& geo_point) const {
  geo_point = ToGeoCoordinates(world_point);
  return true;
}

}  // namespace geo
}  // namespace olp
