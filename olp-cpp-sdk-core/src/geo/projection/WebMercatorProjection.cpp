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

#include <olp/core/geo/projection/WebMercatorProjection.h>

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
double projectLatitude(double latitude) {
  return log(tan(pi * 0.25 + latitude * 0.5)) / pi;
}

const double MAX_LATITUDE = 1.4844222297453323;
const double MIN_LATITUDE = -MAX_LATITUDE;
const double MAX_LONGITUDE = math::Radians(180);
const double MIN_LONGITUDE = -MAX_LONGITUDE;

double clampLatitude(double latitude) {
  return Clamp(latitude, MIN_LATITUDE, MAX_LATITUDE);
}

double projectClampLatitude(double latitude) {
  return projectLatitude(clampLatitude(latitude));
}

// TODO: Clamping makes the projection non-invertible
double unprojectLatitude(double y) { return 2.0 * atan(exp(pi * y)) - half_pi; }

Vector3d toWorld(const GeoCoordinates3d& geo_coords) {
  const GeoCoordinates normalized = geo_coords.GetGeoCoordinates().Normalized();
  const double c = EarthConstants::EquatorialCircumference();

  return {(normalized.GetLongitude() + pi) / (2 * pi) * c,
          (0.5 * (projectClampLatitude(normalized.GetLatitude()) + 1)) * c,
          geo_coords.GetAltitude()};
}

GeoCoordinates3d toGeodetic(const Vector3d& point) {
  const double c = EarthConstants::EquatorialCircumference();
  return {unprojectLatitude(2 * (point.y / c - 0.5)),
          (2 * (point.x / c) - 1) * pi, point.z};
}
}  // namespace

bool WebMercatorProjection::IsEqualTo(const IProjection& other) const {
  return typeId(*this) == typeId(other);
}

GeoRectangle WebMercatorProjection::GetGeoBounds() const {
  return {{MIN_LATITUDE, MIN_LONGITUDE}, {MAX_LATITUDE, MAX_LONGITUDE}};
}

WorldAlignedBox WebMercatorProjection::WorldExtent(
    double minimum_altitude, double maximum_altitude) const {
  WorldCoordinates min(0, 0, minimum_altitude);

  WorldCoordinates max(EarthConstants::EquatorialCircumference(),
                       EarthConstants::EquatorialCircumference(),
                       maximum_altitude);

  return WorldAlignedBox(min, max);
}

bool WebMercatorProjection::Project(const GeoCoordinates3d& geo_point,
                                    WorldCoordinates& world_point) const {
  world_point = toWorld(geo_point);
  return true;
}

bool WebMercatorProjection::Unproject(const WorldCoordinates& world_point,
                                      GeoCoordinates3d& geo_point) const {
  geo_point = toGeodetic(world_point);
  return true;
}

}  // namespace geo
}  // namespace olp
