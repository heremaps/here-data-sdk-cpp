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

#include "olp/core/geo/projection/EquirectangularProjection.h"

#include "olp/core/geo/Types.h"
#include "olp/core/geo/coordinates/GeoCoordinates3d.h"
#include "olp/core/geo/coordinates/GeoRectangle.h"
#include "olp/core/geo/projection/EarthConstants.h"
#include "olp/core/math/AlignedBox.h"
#include "olp/core/math/Math.h"

namespace olp {
namespace geo {
namespace {
constexpr double kWorldToGeoScale = math::two_pi;
constexpr double kGeoToWorldScale = 1.0 / kWorldToGeoScale;

WorldCoordinates ToWorldCoordinates(const GeoCoordinates3d& geo_coordinates) {
  return {(geo_coordinates.GetLongitude() + math::pi) * kGeoToWorldScale,
          (geo_coordinates.GetLatitude() + math::half_pi) * kGeoToWorldScale,
          geo_coordinates.GetAltitude()};
}

GeoCoordinates3d ToGeoCoordinates(const WorldCoordinates& point) {
  return {point.y * kWorldToGeoScale - math::half_pi,
          point.x * kWorldToGeoScale - math::pi, point.z};
}
}  // namespace

GeoRectangle EquirectangularProjection::GetGeoBounds() const {
  return {{-math::half_pi, -math::pi}, {+math::half_pi, +math::pi}};
}

WorldAlignedBox EquirectangularProjection::WorldExtent(
    double minimum_altitude, double maximum_altitude) const {
  return WorldAlignedBox(WorldCoordinates(0, 0, minimum_altitude),
                         WorldCoordinates(1, 0.5, maximum_altitude));
}

bool EquirectangularProjection::Project(const GeoCoordinates3d& geo_point,
                                        WorldCoordinates& world_point) const {
  world_point = ToWorldCoordinates(geo_point);
  return true;
}

bool EquirectangularProjection::Unproject(const WorldCoordinates& world_point,
                                          GeoCoordinates3d& geo_point) const {
  geo_point = ToGeoCoordinates(world_point);
  return true;
}

}  // namespace geo
}  // namespace olp
