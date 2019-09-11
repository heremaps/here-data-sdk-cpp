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
#include <olp/core/geo/coordinates/GeoCoordinates3d.h>
#include <olp/core/geo/projection/EquirectangularProjection.h>

namespace olp {
using namespace math;

namespace geo {
static void TestProjectUnproject(const GeoCoordinates3d& geo,
                                 const WorldCoordinates& world) {
  {
    WorldCoordinates actualWorld;
    EXPECT_TRUE(EquirectangularProjection().Project(geo, actualWorld));
    EXPECT_VECTOR_EQ(world, actualWorld);
  }

  {
    GeoCoordinates3d actualGeo(0, 0, 0);
    EXPECT_TRUE(EquirectangularProjection().Unproject(world, actualGeo));
    EXPECT_GEOCOORDINATES3D_EQ(geo, actualGeo);
  }
}

TEST(EquirectangularProjectionTest, ProjectUnprojectPoint) {
  TestProjectUnproject(GeoCoordinates3d(GeoCoordinates::FromDegrees(0, 0), 0),
                       WorldCoordinates(0.5, 0.25, 0));

  TestProjectUnproject(GeoCoordinates3d(GeoCoordinates::FromDegrees(-90, 0), 0),
                       WorldCoordinates(0.5, 0, 0));

  TestProjectUnproject(GeoCoordinates3d(GeoCoordinates::FromDegrees(90, 0), 0),
                       WorldCoordinates(0.5, 0.5, 0));

  TestProjectUnproject(
      GeoCoordinates3d(GeoCoordinates::FromDegrees(0, -180), 0),
      WorldCoordinates(0, 0.25, 0));

  TestProjectUnproject(GeoCoordinates3d(GeoCoordinates::FromDegrees(0, 180), 0),
                       WorldCoordinates(1, 0.25, 0));

  TestProjectUnproject(GeoCoordinates3d(GeoCoordinates::FromDegrees(0, 0), -10),
                       WorldCoordinates(0.5, 0.25, -10));
}

}  // namespace geo
}  // namespace olp
