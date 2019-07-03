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

#include <olp/core/geo/tiling/HalfQuadTreeSubdivisionScheme.h>
#include <olp/core/geo/tiling/QuadTreeSubdivisionScheme.h>

namespace olp {
namespace geo {
TEST(SubdivisionScheme, QuadTreeSubdivisionScheme) {
  QuadTreeSubdivisionScheme quadTreeSubdivisionScheme;

  uint32_t dimensionX = 1;
  uint32_t dimensionY = 1;

  for (uint32_t level = 0; level != 32; ++level) {
    const auto& levelSize = quadTreeSubdivisionScheme.GetLevelSize(level);
    EXPECT_EQ(dimensionX, levelSize.Width());
    EXPECT_EQ(dimensionY, levelSize.Height());

    const auto& subdivision = quadTreeSubdivisionScheme.GetSubdivisionAt(level);
    dimensionX *= subdivision.Width();
    dimensionY *= subdivision.Height();
  }
}

TEST(SubdivisionScheme, HalfQuadTreeSubdivisionScheme) {
  HalfQuadTreeSubdivisionScheme halfQuadTreeSubdivisionScheme;

  uint32_t dimensionX = 1;
  uint32_t dimensionY = 1;

  for (uint32_t level = 0; level != 32; ++level) {
    const auto& levelSize = halfQuadTreeSubdivisionScheme.GetLevelSize(level);
    EXPECT_EQ(dimensionX, levelSize.Width());
    EXPECT_EQ(dimensionY, levelSize.Height());

    const auto& subdivision =
        halfQuadTreeSubdivisionScheme.GetSubdivisionAt(level);
    dimensionX *= subdivision.Width();
    dimensionY *= subdivision.Height();
  }
}

TEST(SubdivisionScheme, Dimensions) {
  QuadTreeSubdivisionScheme quadTreeSubdivisionScheme;
  HalfQuadTreeSubdivisionScheme halfQuadTreeSubdivisionScheme;

  EXPECT_EQ(halfQuadTreeSubdivisionScheme.GetLevelSize(5).Width(),
            quadTreeSubdivisionScheme.GetLevelSize(5).Width());

  EXPECT_EQ(halfQuadTreeSubdivisionScheme.GetLevelSize(5).Height() * 2,
            quadTreeSubdivisionScheme.GetLevelSize(5).Height());

  EXPECT_EQ(1U, halfQuadTreeSubdivisionScheme.GetLevelSize(0).Height());
  EXPECT_EQ(1U, halfQuadTreeSubdivisionScheme.GetLevelSize(1).Height());
}

}  // namespace geo
}  // namespace olp
