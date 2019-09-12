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

#include <olp/core/geo/projection/EarthConstants.h>
#include <olp/core/geo/projection/SphereProjection.h>
#include <olp/core/math/Math.h>

#include <random>

using namespace olp::geo;
using namespace olp::math;
using namespace testing;

const auto EARTH_ORIGIN = Vector3d{0, 0, 0};
const double EARTH_RADIUS = EarthConstants::EquatorialRadius();

class SphereProjectionTest : public Test {
 public:
  SphereProjectionTest() : loopCount{100} {}

  template <typename F>
  void testLoop(F&& f) {
    for (size_t i = 0; i < loopCount; ++i) {
      f();
    }
  }

  std::mt19937 rng;
  size_t loopCount;
  SphereProjection projection;
};

TEST_F(SphereProjectionTest, ProjectUnprojectPoint) {
  GeoCoordinates3d geoCoordinate = GeoCoordinates3d(
      GeoCoordinates::FromDegrees(37.8178183439856, -122.4410209359072), 12.0f);
  WorldCoordinates worldPoint;
  GeoCoordinates3d geoPoint2;
  EXPECT_TRUE(SphereProjection().Project(geoCoordinate, worldPoint));
  EXPECT_TRUE(SphereProjection().Unproject(worldPoint, geoPoint2));
  EXPECT_GEOCOORDINATES3D_EQ(geoCoordinate, geoPoint2);
}

TEST_F(SphereProjectionTest, ProjectPoint) {
  WorldCoordinates worldPoint;

  const GeoCoordinates3d geoPointX(GeoCoordinates::FromDegrees(0, 0), 0);
  const WorldCoordinates worldPointX(EarthConstants::EquatorialRadius(), 0, 0);

  const GeoCoordinates3d geoPointY(GeoCoordinates::FromDegrees(0, 90), 0);
  const WorldCoordinates worldPointY(0, EarthConstants::EquatorialRadius(), 0);

  const GeoCoordinates3d geoPointZ(GeoCoordinates::FromDegrees(90, 0), 0);
  const WorldCoordinates worldPointZ(0, 0, EarthConstants::EquatorialRadius());

  EXPECT_TRUE(SphereProjection().Project(geoPointX, worldPoint));
  EXPECT_VECTOR_EQ(worldPointX, worldPoint);

  EXPECT_TRUE(SphereProjection().Project(geoPointY, worldPoint));
  EXPECT_VECTOR_EQ(worldPointY, worldPoint);

  EXPECT_TRUE(SphereProjection().Project(geoPointZ, worldPoint));
  EXPECT_VECTOR_EQ(worldPointZ, worldPoint);

  const GeoCoordinates3d geoPointXAltitude(GeoCoordinates::FromDegrees(0, 0),
                                           12);
  const WorldCoordinates worldPointXAltitude(
      EarthConstants::EquatorialRadius() + 12, 0, 0);

  EXPECT_TRUE(SphereProjection().Project(geoPointXAltitude, worldPoint));
  EXPECT_VECTOR_EQ(worldPointXAltitude, worldPoint);
}

TEST_F(SphereProjectionTest, UnprojectPoint) {
  GeoCoordinates3d geoPoint(0, 0, 0);

  const GeoCoordinates3d geoPointX(GeoCoordinates::FromDegrees(0, 0), 0);
  const WorldCoordinates worldPointX(EarthConstants::EquatorialRadius(), 0, 0);

  const GeoCoordinates3d geoPointY(GeoCoordinates::FromDegrees(0, 90), 0);
  const WorldCoordinates worldPointY(0, EarthConstants::EquatorialRadius(), 0);

  const GeoCoordinates3d geoPointZ(GeoCoordinates::FromDegrees(90, 0), 0);
  const WorldCoordinates worldPointZ(0, 0, EarthConstants::EquatorialRadius());

  EXPECT_TRUE(SphereProjection().Unproject(worldPointX, geoPoint));
  EXPECT_GEOCOORDINATES3D_EQ(geoPointX, geoPoint);

  EXPECT_TRUE(SphereProjection().Unproject(worldPointY, geoPoint));
  EXPECT_GEOCOORDINATES3D_EQ(geoPointY, geoPoint);

  EXPECT_TRUE(SphereProjection().Unproject(worldPointZ, geoPoint));
  EXPECT_GEOCOORDINATES3D_EQ(geoPointZ, geoPoint);

  const GeoCoordinates3d geoPointXAltitude(GeoCoordinates::FromDegrees(0, 0),
                                           12);
  const WorldCoordinates worldPointXAltitude(
      EarthConstants::EquatorialRadius() + 12, 0, 0);

  EXPECT_TRUE(SphereProjection().Unproject(worldPointXAltitude, geoPoint));
  EXPECT_GEOCOORDINATES3D_EQ(geoPointXAltitude, geoPoint);

  EXPECT_TRUE(
      SphereProjection().Unproject(WorldCoordinates(0, 0, 0), geoPoint));
  EXPECT_DOUBLE_EQ(-EarthConstants::EquatorialRadius(), geoPoint.GetAltitude());
}
