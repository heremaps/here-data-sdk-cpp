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

#include "../testutil/CompareGeoCoordinates.h"
#include "../testutil/CompareGeoCoordinates3d.h"
#include <gtest/gtest.h>

#include <olp/core/geo/coordinates/GeoRectangle.h>
#include <olp/core/geo/projection/EarthConstants.h>
#include <olp/core/geo/projection/WebMercatorProjection.h>

namespace olp {
using namespace math;

namespace geo {
static void TestProjectUnproject(const GeoCoordinates3d& geo,
                                 const WorldCoordinates& world) {
  {
    WorldCoordinates actualWorld;
    EXPECT_TRUE(WebMercatorProjection().Project(geo, actualWorld));
    EXPECT_VECTOR_EQ(world, actualWorld);
  }

  {
    GeoCoordinates3d actualGeo(0, 0, 0);
    EXPECT_TRUE(WebMercatorProjection().Unproject(world, actualGeo));
    const GeoCoordinates3d normalizedGeo(geo.GetGeoCoordinates().Normalized(),
                                         geo.GetAltitude());
    EXPECT_GEOCOORDINATES3D_EQ(normalizedGeo, actualGeo);
  }
}

TEST(WebMercatorProjectionTest, ProjectUnprojectPoint) {
  const double r = EarthConstants::EquatorialCircumference();
  const auto georect = WebMercatorProjection().GetGeoBounds();

  TestProjectUnproject(GeoCoordinates3d(0, 0, 0),
                       WorldCoordinates(0.5 * r, 0.5 * r, 0));

  TestProjectUnproject(
      GeoCoordinates3d(georect.SouthWest().GetLatitude(), 0, 0),
      WorldCoordinates(0.5 * r, 0, 0));

  TestProjectUnproject(
      GeoCoordinates3d(georect.NorthEast().GetLatitude(), 0, 0),
      WorldCoordinates(0.5 * r, 1 * r, 0));

  TestProjectUnproject(GeoCoordinates3d(GeoCoordinates::FromDegrees(0, 180), 0),
                       WorldCoordinates(0, 0.5 * r, 0));

  TestProjectUnproject(
      GeoCoordinates3d(GeoCoordinates::FromDegrees(0, -180), 0),
      WorldCoordinates(0, 0.5 * r, 0));

  TestProjectUnproject(GeoCoordinates3d(0, 0, -10),
                       WorldCoordinates(0.5 * r, 0.5 * r, -10));
}

}  // namespace geo
}  // namespace olp
