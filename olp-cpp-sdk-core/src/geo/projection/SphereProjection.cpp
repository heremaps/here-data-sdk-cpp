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

#include <olp/core/geo/projection/SphereProjection.h>

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/geo/coordinates/GeoCoordinates3d.h>
#include <olp/core/geo/coordinates/GeoRectangle.h>
#include <olp/core/geo/projection/EarthConstants.h>
#include <olp/core/math/AlignedBox.h>
#include <olp/core/math/Math.h>

using namespace core;

namespace olp {

using namespace math;

namespace geo {
namespace {

static Vector3d worldNormal(const GeoCoordinates& geo_point) {
  const double cosLatitude = cos(geo_point.GetLatitude());
  return Vector3d(cos(geo_point.GetLongitude()) * cosLatitude,
                  sin(geo_point.GetLongitude()) * cosLatitude,
                  sin(geo_point.GetLatitude()));
}

Vector3d toWorld(const GeoCoordinates3d& geo_coords) {
  const double radius =
      EarthConstants::EquatorialRadius() + geo_coords.GetAltitude();
  return worldNormal(geo_coords.GetGeoCoordinates()) * radius;
}

GeoCoordinates3d toGeodetic(const Vector3d& point) {
  const double parallelRadiusSq = point.x * point.x + point.y * point.y;
  const double parallelRadius = sqrt(parallelRadiusSq);

  // World-space origin maps to any lat, lng
  const double v = point.z / parallelRadius;
  if (isnan(v)) {
    return {0, 0, -EarthConstants::EquatorialRadius()};
  }

  const double radius = sqrt(parallelRadiusSq + point.z * point.z);

  return {atan(v), atan2(point.y, point.x),
          radius - EarthConstants::EquatorialRadius()};
}

}  // namespace

bool SphereProjection::IsEqualTo(const IProjection& other) const {
  return typeId(*this) == typeId(other);
}

GeoRectangle SphereProjection::GetGeoBounds() const {
  return {{-half_pi, -pi}, {+half_pi, +pi}};
}

WorldAlignedBox SphereProjection::WorldExtent(double /*minimum_altitude*/,
                                              double maximum_altitude) const {
  auto radius = Vector3d(1, 1, 1) *
                (EarthConstants::EquatorialRadius() + maximum_altitude);
  return WorldAlignedBox(-radius, radius);
}

bool SphereProjection::Project(const GeoCoordinates3d& geo_point,
                               WorldCoordinates& world_point) const {
  world_point = toWorld(geo_point);
  return true;
}

bool SphereProjection::Unproject(const WorldCoordinates& world_point,
                                 GeoCoordinates3d& geo_point) const {
  geo_point = toGeodetic(world_point);
  return true;
}

}  // namespace geo
}  // namespace olp
