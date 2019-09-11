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
#include <olp/core/geo/coordinates/GeoPoint.h>

namespace olp {
namespace geo {
// used to output test failures
std::ostream& operator<<(std::ostream& os, const geo::GeoPoint& pt) {
  return os << "(" << pt.x << ", " << pt.y << ")";
}

TEST(GeoPointTest, Operators) {
  GeoPoint pt1 = {1, 2};
  GeoPoint pt2 = pt1;
  GeoPoint pt3 = {2, 4};

  EXPECT_EQ(pt1, pt2);
  EXPECT_NE(pt1, pt3);

  pt1 += pt2;
  EXPECT_EQ(pt1, pt3);
}

}  // namespace geo
}  // namespace olp
