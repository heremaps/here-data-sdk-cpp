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

#include <olp/core/geo/coordinates/GeoRectangle.h>
#include <olp/core/math/Math.h>
#include "../testutil/CompareGeoCoordinates.h"
#include "../testutil/CompareGeoRectangle.h"

namespace olp {
namespace geo {

TEST(GeoRectangleTest, Constructor) {
  const GeoCoordinates min(0.0, 0.1);
  const GeoCoordinates max(0.1, 0.2);
  GeoRectangle rectangle(min, max);

  EXPECT_EQ(min, rectangle.SouthWest());
  EXPECT_EQ(GeoCoordinates(min.GetLatitude(), max.GetLongitude()),
            rectangle.SouthEast());
  EXPECT_EQ(GeoCoordinates(max.GetLatitude(), min.GetLongitude()),
            rectangle.NorthWest());
  EXPECT_EQ(max, rectangle.NorthEast());
}

TEST(GeoRectangleTest, IsEmpty) {
  EXPECT_TRUE(GeoRectangle().IsEmpty());
  EXPECT_FALSE(
      GeoRectangle(GeoCoordinates(0, 0), GeoCoordinates(1, 1)).IsEmpty());
}

TEST(GeoRectangleTest, Center) {
  EXPECT_GEOCOORDINATES_EQ(
      GeoCoordinates(0.5, 0.5),
      GeoRectangle(GeoCoordinates(0, 0), GeoCoordinates(1, 1)).Center());

  EXPECT_GEOCOORDINATES_EQ(GeoCoordinates::FromDegrees(0, 175),
                           GeoRectangle(GeoCoordinates::FromDegrees(-10, 160),
                                        GeoCoordinates::FromDegrees(10, -170))
                               .Center());
}

TEST(GeoRectangleTest, Dimensions) {
  const GeoCoordinates min(0.0, 0.1);
  const GeoCoordinates max(0.1, 0.2);
  GeoRectangle rectangle(min, max);

  EXPECT_DOUBLE_EQ(max.GetLatitude() - min.GetLatitude(),
                   rectangle.LatitudeSpan());
  EXPECT_DOUBLE_EQ(max.GetLongitude() - min.GetLongitude(),
                   rectangle.LongitudeSpan());
}

TEST(GeoRectangleTest, Containment) {
  const GeoCoordinates min(0.0, 0.1);
  const GeoCoordinates center(0.05, 0.15);
  const GeoCoordinates max(0.1, 0.2);
  const GeoCoordinates outside(0.05, 0.8);
  GeoRectangle rectangle(min, max);

  EXPECT_TRUE(rectangle.Contains(min));
  EXPECT_TRUE(rectangle.Contains(center));
  EXPECT_TRUE(rectangle.Contains(max));
  EXPECT_FALSE(rectangle.Contains(outside));

  EXPECT_FALSE(rectangle.Overlaps(
      GeoRectangle(GeoCoordinates(-0.2, -0.2), GeoCoordinates(-0.1, -0.1))));

  EXPECT_TRUE(rectangle.Overlaps(
      GeoRectangle(GeoCoordinates(0, 0), GeoCoordinates(0.05, 0.15))));
}

TEST(GeoRectangleTest, OperatorEqual) {
  EXPECT_EQ(GeoRectangle(GeoCoordinates(1, 2), GeoCoordinates(10, 20)),
            GeoRectangle(GeoCoordinates(1, 2), GeoCoordinates(10, 20)));
}

TEST(GeoRectangleTest, OperatorNotEqual) {
  EXPECT_NE(GeoRectangle(GeoCoordinates(0, 0), GeoCoordinates(1, 1)),
            GeoRectangle(GeoCoordinates(0, 0), GeoCoordinates(1, 2)));
  EXPECT_NE(GeoRectangle(GeoCoordinates(0, 0), GeoCoordinates(1, 1)),
            GeoRectangle(GeoCoordinates(1, 0), GeoCoordinates(1, 1)));
}

TEST(GeoRectangleTest, BooleanUnion) {
  // Non-connected
  {
    const GeoRectangle rectangle1(GeoCoordinates(0.0, 0.0),
                                  GeoCoordinates(0.1, 0.1));
    const GeoRectangle rectangle2(GeoCoordinates(1.0, 1.0),
                                  GeoCoordinates(1.1, 1.1));

    const GeoRectangle out = rectangle1.BooleanUnion(rectangle2);
    EXPECT_GEOCOORDINATES_EQ(rectangle1.SouthWest(), out.SouthWest());
    EXPECT_GEOCOORDINATES_EQ(rectangle2.NorthEast(), out.NorthEast());
  }

  // Overlapping
  {
    const GeoRectangle rectangle1(GeoCoordinates(0.0, 0.0),
                                  GeoCoordinates(2.0, 2.0));
    const GeoRectangle rectangle2(GeoCoordinates(1.0, 1.0),
                                  GeoCoordinates(2.1, 2.1));

    const GeoRectangle out = rectangle1.BooleanUnion(rectangle2);
    EXPECT_GEOCOORDINATES_EQ(rectangle1.SouthWest(), out.SouthWest());
    EXPECT_GEOCOORDINATES_EQ(rectangle2.NorthEast(), out.NorthEast());
  }

  // Fully contained
  {
    const GeoRectangle rectangle1(GeoCoordinates(0.0, 0.0),
                                  GeoCoordinates(2.0, 2.0));
    const GeoRectangle rectangle2(GeoCoordinates(1.0, 1.0),
                                  GeoCoordinates(1.1, 1.1));

    const GeoRectangle out = rectangle1.BooleanUnion(rectangle2);
    EXPECT_GEOCOORDINATES_EQ(rectangle1.SouthWest(), out.SouthWest());
    EXPECT_GEOCOORDINATES_EQ(rectangle1.NorthEast(), out.NorthEast());
  }

  // Wrap around
  {
    const GeoRectangle rectangle1(GeoCoordinates(0.0, 1.0),
                                  GeoCoordinates(1.0, 0));
    const GeoRectangle rectangle2(GeoCoordinates(0.0, 0.0),
                                  GeoCoordinates(1.0, 1.0));

    const GeoRectangle out = rectangle1.BooleanUnion(rectangle2);
    EXPECT_DOUBLE_EQ(1.0, out.LatitudeSpan());
    EXPECT_DOUBLE_EQ(math::two_pi, out.LongitudeSpan());
  }
}

TEST(GeoRectangleTest, GrowToContain) {
  const GeoCoordinates insidePoint(0.05, 0.05);
  const GeoCoordinates southPoint(-0.1, 0.05);
  const GeoCoordinates northPoint(0.2, 0.05);
  const GeoCoordinates westPoint(0.05, -0.1);
  const GeoCoordinates eastPoint(0.05, 0.2);

  GeoRectangle emptyRectangle;
  EXPECT_TRUE(emptyRectangle.IsEmpty());
  emptyRectangle.GrowToContain(southPoint);
  EXPECT_FALSE(emptyRectangle.IsEmpty());
  EXPECT_GEORECTANGLE_EQ(GeoRectangle(southPoint, southPoint), emptyRectangle);

  GeoRectangle rectangle(GeoCoordinates(0.0, 0.0), GeoCoordinates(0.1, 0.1));

  rectangle.GrowToContain(insidePoint);
  EXPECT_GEORECTANGLE_EQ(
      GeoRectangle(GeoCoordinates(0.0, 0.0), GeoCoordinates(0.1, 0.1)),
      rectangle);
  rectangle.GrowToContain(southPoint);
  EXPECT_GEORECTANGLE_EQ(
      GeoRectangle(GeoCoordinates(-0.1, 0.0), GeoCoordinates(0.1, 0.1)),
      rectangle);
  rectangle.GrowToContain(northPoint);
  EXPECT_GEORECTANGLE_EQ(
      GeoRectangle(GeoCoordinates(-0.1, 0.0), GeoCoordinates(0.2, 0.1)),
      rectangle);
  rectangle.GrowToContain(westPoint);
  EXPECT_GEORECTANGLE_EQ(
      GeoRectangle(GeoCoordinates(-0.1, -0.1), GeoCoordinates(0.2, 0.1)),
      rectangle);
  rectangle.GrowToContain(eastPoint);
  EXPECT_GEORECTANGLE_EQ(
      GeoRectangle(GeoCoordinates(-0.1, -0.1), GeoCoordinates(0.2, 0.2)),
      rectangle);
}

}  // namespace geo
}  // namespace olp
