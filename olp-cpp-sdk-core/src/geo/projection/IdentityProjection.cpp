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

#include "olp/core/geo/projection/IdentityProjection.h"

#include "olp/core/geo/Types.h"
#include "olp/core/geo/coordinates/GeoCoordinates.h"
#include "olp/core/geo/coordinates/GeoCoordinates3d.h"
#include "olp/core/geo/coordinates/GeoRectangle.h"
#include "olp/core/math/AlignedBox.h"
#include "olp/core/math/Math.h"

namespace olp {
namespace geo {
namespace {

WorldCoordinates ToWorldCoordinates(const GeoCoordinates3d& geo_coordinates) {
  return {geo_coordinates.GetLongitude(), geo_coordinates.GetLatitude(),
          geo_coordinates.GetAltitude()};
}
GeoCoordinates3d ToGeoCoordinates(const WorldCoordinates& point) {
  return {point.y, point.x, point.z};
}
}  // namespace

GeoRectangle IdentityProjection::GetGeoBounds() const {
  return {{-math::half_pi, -math::pi}, {+math::half_pi, +math::pi}};
}

WorldAlignedBox IdentityProjection::WorldExtent(double minimum_altitude,
                                                double maximum_altitude) const {
  WorldCoordinates min(-math::pi, -math::half_pi, minimum_altitude);
  WorldCoordinates max(math::pi, math::half_pi, maximum_altitude);
  return WorldAlignedBox(min, max);
}

bool IdentityProjection::Project(const GeoCoordinates3d& geo_point,
                                 WorldCoordinates& world_point) const {
  world_point = ToWorldCoordinates(geo_point);
  return true;
}

bool IdentityProjection::Unproject(const WorldCoordinates& world_point,
                                   GeoCoordinates3d& geo_point) const {
  geo_point = ToGeoCoordinates(world_point);
  return true;
}

}  // namespace geo
}  // namespace olp
