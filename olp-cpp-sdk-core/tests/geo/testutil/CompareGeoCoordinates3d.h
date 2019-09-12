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

#pragma once

#include <gtest/gtest.h>

#include <olp/core/geo/coordinates/GeoCoordinates3d.h>

namespace olp {
namespace geo {
inline void EXPECT_GEOCOORDINATES3D_EQ(const GeoCoordinates3d& expected,
                                       const GeoCoordinates3d& actual) {
  EXPECT_DOUBLE_EQ(expected.GetLatitude(), actual.GetLatitude());
  EXPECT_DOUBLE_EQ(expected.GetLongitude(), actual.GetLongitude());
  EXPECT_NEAR(expected.GetAltitude(), actual.GetAltitude(), 1e-5);
}

}  // namespace geo
}  // namespace olp
