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

#include <gtest/gtest.h>

#include <cmath>

#include <olp/core/geo/coordinates/GeoCoordinates3d.h>

namespace olp {
namespace geo {
TEST(GeoCoordinates3dTest, Constructor) {
  const double latitude = 0.1;
  const double longitude = 0.2;
  const double altitude = 12.1;

  const GeoCoordinates3d geoPoint(latitude, longitude, altitude);

  EXPECT_DOUBLE_EQ(latitude, geoPoint.GetLatitude());
  EXPECT_DOUBLE_EQ(longitude, geoPoint.GetLongitude());
  EXPECT_DOUBLE_EQ(altitude, geoPoint.GetAltitude());
}

TEST(GeoCoordinates3dTest, ConstructorDegrees) {
  const double latitude = 0.1;
  const double longitude = 0.2;
  const double altitude = 12.1;

  const GeoCoordinates3d geoPoint(latitude, longitude, altitude, DegreeType());

  EXPECT_DOUBLE_EQ(latitude, geoPoint.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(longitude, geoPoint.GetLongitudeDegrees());
  EXPECT_DOUBLE_EQ(altitude, geoPoint.GetAltitude());
}

TEST(GeoCoordinates3dTest, GeoCoordinates) {
  const double latitude = 0.1;
  const double longitude = 0.2;
  const double altitude = 12.1;
  const GeoCoordinates geoCoord2d(latitude, longitude);
  const GeoCoordinates geoCoord2d2(latitude + 0.1, longitude - 0.1);

  GeoCoordinates3d geoPoint(geoCoord2d, altitude);

  EXPECT_DOUBLE_EQ(geoCoord2d.GetLatitude(),
                   geoPoint.GetGeoCoordinates().GetLatitude());
  EXPECT_DOUBLE_EQ(geoCoord2d.GetLongitude(),
                   geoPoint.GetGeoCoordinates().GetLongitude());
  EXPECT_DOUBLE_EQ(altitude, geoPoint.GetAltitude());

  geoPoint.SetGeoCoordinates(geoCoord2d2);
  EXPECT_DOUBLE_EQ(geoCoord2d2.GetLatitude(),
                   geoPoint.GetGeoCoordinates().GetLatitude());
  EXPECT_DOUBLE_EQ(geoCoord2d2.GetLongitude(),
                   geoPoint.GetGeoCoordinates().GetLongitude());
  EXPECT_DOUBLE_EQ(altitude, geoPoint.GetAltitude());
}

TEST(GeoCoordinates3dTest, Setters) {
  GeoCoordinates3d geoCoords(0, 0, 0);

  EXPECT_DOUBLE_EQ(0, geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitude());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetAltitude());

  geoCoords.SetLatitude(0.1);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitude());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetAltitude());

  geoCoords.SetLongitude(0.2);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(0.2, geoCoords.GetLongitude());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetAltitude());

  geoCoords.SetAltitude(0.3);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(0.2, geoCoords.GetLongitude());
  EXPECT_DOUBLE_EQ(0.3, geoCoords.GetAltitude());
}

TEST(GeoCoordinates3dTest, SettersDegrees) {
  GeoCoordinates3d geoCoords(0, 0, 0, DegreeType());

  EXPECT_DOUBLE_EQ(0, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitudeDegrees());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetAltitude());

  geoCoords.setLatitudeDegrees(0.1);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitudeDegrees());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetAltitude());

  geoCoords.SetLongitudeDegrees(0.2);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(0.2, geoCoords.GetLongitudeDegrees());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetAltitude());

  geoCoords.SetAltitude(0.3);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(0.2, geoCoords.GetLongitudeDegrees());
  EXPECT_DOUBLE_EQ(0.3, geoCoords.GetAltitude());
}

TEST(GeoCoordinates3dTest, Valid) {
  GeoCoordinates3d geoCoords;
  EXPECT_FALSE(geoCoords.IsValid());

  geoCoords.SetAltitude(100);
  EXPECT_FALSE(geoCoords.IsValid());

  geoCoords.SetLatitude(0.75);
  EXPECT_FALSE(geoCoords.IsValid());

  geoCoords.SetLongitude(0.5);
  EXPECT_TRUE(geoCoords.IsValid());

  geoCoords.SetAltitude(NAN);
  EXPECT_FALSE(geoCoords.IsValid());
}

}  // namespace geo
}  // namespace olp
