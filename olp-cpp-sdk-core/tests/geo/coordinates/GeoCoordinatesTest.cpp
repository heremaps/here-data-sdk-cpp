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

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/math/Math.h>

namespace olp {
namespace geo {
// used to output test failures
std::ostream& operator<<(std::ostream& os, const geo::GeoCoordinates& pt) {
  return os << "(" << pt.GetLatitude() << ", " << pt.GetLongitude() << ")";
}

TEST(GeoCoordinatesTest, Constructor) {
  const double latitude = 0.1;
  const double longitude = 0.2;
  const GeoCoordinates geoPoint(latitude, longitude);

  EXPECT_EQ(latitude, geoPoint.GetLatitude());
  EXPECT_EQ(longitude, geoPoint.GetLongitude());
}

TEST(GeoCoordinatesTest, Degrees) {
  const double latitude = 0.1;
  const double longitude = 0.2;
  GeoCoordinates geoCoords = GeoCoordinates::FromDegrees(latitude, longitude);

  EXPECT_DOUBLE_EQ(latitude, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(longitude, geoCoords.GetLongitudeDegrees());
  EXPECT_DOUBLE_EQ(math::Radians(latitude), geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(math::Radians(longitude), geoCoords.GetLongitude());
}

TEST(GeoCoordinatesTest, DegreesConstructor) {
  const double latitude = 0.1;
  const double longitude = 0.2;

  GeoCoordinates geoCoords(latitude, longitude, DegreeType());
  EXPECT_DOUBLE_EQ(latitude, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(longitude, geoCoords.GetLongitudeDegrees());
  EXPECT_DOUBLE_EQ(math::Radians(latitude), geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(math::Radians(longitude), geoCoords.GetLongitude());
}

TEST(GeoCoordinatesTest, Setters) {
  GeoCoordinates geoCoords(0, 0);

  EXPECT_DOUBLE_EQ(0, geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitude());

  geoCoords.SetLatitude(0.1);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitude());

  geoCoords.SetLongitude(0.2);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitude());
  EXPECT_DOUBLE_EQ(0.2, geoCoords.GetLongitude());
}

TEST(GeoCoordinatesTest, DegreesSetters) {
  GeoCoordinates geoCoords(0, 0, DegreeType());

  EXPECT_DOUBLE_EQ(0, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitudeDegrees());

  geoCoords.SetLatitudeDegrees(0.1);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(0, geoCoords.GetLongitudeDegrees());

  geoCoords.SetLongitudeDegrees(0.2);
  EXPECT_DOUBLE_EQ(0.1, geoCoords.GetLatitudeDegrees());
  EXPECT_DOUBLE_EQ(0.2, geoCoords.GetLongitudeDegrees());
}

TEST(GeoCoordinatesTest, GeoPoint) {
  {
    const GeoPoint pt = {0, 0};
    const GeoCoordinates geoCoords = GeoCoordinates::FromGeoPoint(pt);
    EXPECT_DOUBLE_EQ(math::Radians(-90.0), geoCoords.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(-180.0), geoCoords.GetLongitude());
    EXPECT_EQ(pt, geoCoords.ToGeoPoint());
  }
  {
    const GeoPoint pt{1, 1};
    const GeoCoordinates geoCoords = GeoCoordinates::FromGeoPoint(pt);
    EXPECT_EQ(pt, geoCoords.ToGeoPoint());
  }
  {
    const GeoPoint pt{2, 2};
    const GeoCoordinates geoCoords = GeoCoordinates::FromGeoPoint(pt);
    EXPECT_EQ(pt, geoCoords.ToGeoPoint());
  }
}

TEST(GeoCoordinatesTest, Normalize) {
  // Note: The normalize operation can cause a small loss of precision.
  // EXPECT_NEAR with an explicit epsilon is used instead of EXPECT_DOUBLE_EQ in
  // these cases.

  // Already normal
  {
    GeoCoordinates normalized(GeoCoordinates(0, 0).Normalized());
    EXPECT_DOUBLE_EQ(0, normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(0, normalized.GetLongitude());
  }

  // 5 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(5.0), math::Radians(0.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(5.0), normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // 95 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(95.0), math::Radians(0.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(90.0), normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // 185 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(185.0), math::Radians(0.0)).Normalized());
    EXPECT_NEAR(math::Radians(90.0), normalized.GetLatitude(), math::epsilon);
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // 275 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(275.0), math::Radians(0.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(90.0), normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // 365 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(365.0), math::Radians(0.0)).Normalized());
    EXPECT_NEAR(math::Radians(90.0), normalized.GetLatitude(), math::epsilon);
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // 725 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(725.0), math::Radians(0.0)).Normalized());
    EXPECT_NEAR(math::Radians(90.0), normalized.GetLatitude(), math::epsilon);
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // -5 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(-5.0), math::Radians(0.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(-5.0), normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // -95 degrees latitude
  {
    GeoCoordinates normalized(
        GeoCoordinates(math::Radians(-95.0), math::Radians(0.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(-90.0), normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), normalized.GetLongitude());
  }

  // -185 degrees latitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(-185.0), math::Radians(0.0)).Normalized());
    EXPECT_NEAR(math::Radians(-90.0), Normalized.GetLatitude(), math::epsilon);
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLongitude());
  }

  // -275 degrees latitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(-275.0), math::Radians(0.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(-90.0), Normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLongitude());
  }

  // -365 degrees latitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(-365.0), math::Radians(0.0)).Normalized());
    EXPECT_NEAR(math::Radians(-90.0), Normalized.GetLatitude(), math::epsilon);
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLongitude());
  }

  // -725 degrees latitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(-725.0), math::Radians(0.0)).Normalized());
    EXPECT_NEAR(math::Radians(-90.0), Normalized.GetLatitude(), math::epsilon);
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLongitude());
  }

  // 5 degrees longitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(0.0), math::Radians(5.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(5.0), Normalized.GetLongitude());
  }

  // 185 degrees longitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(0.0), math::Radians(185.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(-175.0), Normalized.GetLongitude());
  }

  // 365 degrees longitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(0.0), math::Radians(365.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLatitude());
    EXPECT_NEAR(math::Radians(5.0), Normalized.GetLongitude(), math::epsilon);
  }

  // -5 degrees longitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(0.0), math::Radians(-5.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(-5.0), Normalized.GetLongitude());
  }

  // -185 degrees longitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(0.0), math::Radians(-185.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(175.0), Normalized.GetLongitude());
  }

  // -365 degrees longitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(0.0), math::Radians(-365.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(0.0), Normalized.GetLatitude());
    EXPECT_NEAR(math::Radians(-5.0), Normalized.GetLongitude(), math::epsilon);
  }

  // -91 degrees latitude, 1 degree longitude
  {
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(-91.0), math::Radians(1.0)).Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(-90.0), Normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(1.0), Normalized.GetLongitude());
  }

  {
    /* already Normalized*/
    GeoCoordinates topLeft(math::Radians(90.0), math::Radians(-180.0));
    EXPECT_EQ(topLeft.Normalized(), topLeft.Normalized().Normalized());
  }

  {
    /* already Normalized*/
    GeoCoordinates Normalized(
        GeoCoordinates(math::Radians(-90.0), math::Radians(-180.0))
            .Normalized());
    EXPECT_DOUBLE_EQ(math::Radians(-90.0), Normalized.GetLatitude());
    EXPECT_DOUBLE_EQ(math::Radians(-180.0), Normalized.GetLongitude());
  }

  // A specific know failure case
  {
    double lat = -0.78571278946767165;
    double lon = -3.1420368548861641;
    geo::GeoCoordinates pos(lat, lon);
    geo::GeoCoordinates posNorm = pos.Normalized();
    EXPECT_NEAR(posNorm.GetLatitude(), lat, math::epsilon);
    // Longitude should be a little bit less (around 0.0005 less) than pi.
    EXPECT_NEAR(posNorm.GetLongitude(), math::two_pi + lon, math::epsilon);
  }
}

TEST(GeoCoordinatesTest, Valid) {
  GeoCoordinates geoCoords;
  EXPECT_FALSE(geoCoords.IsValid());

  GeoCoordinates Normalized = geoCoords.Normalized();
  EXPECT_FALSE(Normalized.IsValid());

  geoCoords.SetLatitude(100);
  EXPECT_FALSE(geoCoords.IsValid());

  geoCoords.SetLongitude(100);
  EXPECT_TRUE(geoCoords.IsValid());

  Normalized = geoCoords.Normalized();
  EXPECT_TRUE(Normalized.IsValid());

  GeoCoordinates coordinates(1.0, 1.0);
  EXPECT_TRUE(coordinates.IsValid());

  coordinates.SetLatitude(NAN);
  EXPECT_FALSE(coordinates.IsValid());
}

}  // namespace geo
}  // namespace olp
