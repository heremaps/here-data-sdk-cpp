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

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/geo/coordinates/GeoRectangle.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/geo/tiling/TileKeyUtils.h>
#include <olp/core/geo/tiling/TilingSchemeRegistry.h>

#include <gtest/gtest.h>

#include <queue>
#include <vector>

namespace olp {
namespace geo {
TEST(TileKeyUtilsTest, GeoCoordinatesToTileKey) {
  const HalfQuadTreeEquirectangularTilingScheme tilingScheme;
  const GeoCoordinates berlin(GeoCoordinates::FromDegrees(52.5167, 13.3833));

  {
    TileKey tileKey =
        TileKeyUtils::GeoCoordinatesToTileKey(tilingScheme, berlin, 0);
    EXPECT_TRUE(tileKey.IsValid());
    EXPECT_EQ(TileKey::FromRowColumnLevel(0, 0, 0), tileKey);
  }

  {
    TileKey tileKey =
        TileKeyUtils::GeoCoordinatesToTileKey(tilingScheme, berlin, 1);
    EXPECT_TRUE(tileKey.IsValid());
    EXPECT_EQ(TileKey::FromRowColumnLevel(0, 1, 1), tileKey);
  }

  {
    TileKey tileKey =
        TileKeyUtils::GeoCoordinatesToTileKey(tilingScheme, berlin, 14);
    EXPECT_TRUE(tileKey.IsValid());
    EXPECT_EQ(TileKey::FromRowColumnLevel(6486, 8801, 14), tileKey);
  }

  {
    TileKey prevTileKey;
    for (int32_t level = 20; level >= 0; --level) {
      const TileKey tileKey =
          TileKeyUtils::GeoCoordinatesToTileKey(tilingScheme, berlin, level);
      EXPECT_TRUE(tileKey.IsValid());

      if (prevTileKey.IsValid()) {
        EXPECT_EQ(tileKey, prevTileKey.ChangedLevelTo(level));
      }

      prevTileKey = tileKey;
    }
  }
}

TEST(TileKeyUtilsTest, GeoRectangleToTileKeys) {
  const HalfQuadTreeEquirectangularTilingScheme tilingScheme;
  const GeoCoordinates berlinCenter(
      GeoCoordinates::FromDegrees(52.5167, 13.3833));
  const double halfSize = 0.005;
  const GeoRectangle berlinCenterArea(
      GeoCoordinates(berlinCenter.GetLatitude() - halfSize,
                     berlinCenter.GetLongitude() - halfSize),
      GeoCoordinates(berlinCenter.GetLatitude() + halfSize,
                     berlinCenter.GetLongitude() + halfSize));

  {
    const auto tileKeys =
        TileKeyUtils::GeoRectangleToTileKeys(tilingScheme, GeoRectangle(), 15);
    EXPECT_TRUE(tileKeys.empty());
  }

  {
    std::vector<TileKey> prevTileKeys;
    for (int32_t level = 15; level >= 0; --level) {
      std::vector<TileKey> tileKeys = TileKeyUtils::GeoRectangleToTileKeys(
          tilingScheme, berlinCenterArea, level);
      EXPECT_TRUE(!tileKeys.empty());

      if (!prevTileKeys.empty()) {
        for (const TileKey& tileKey : prevTileKeys)
          EXPECT_NE(std::count(tileKeys.cbegin(), tileKeys.cend(),
                               tileKey.ChangedLevelTo(level)),
                    0);
      }

      prevTileKeys = std::move(tileKeys);
    }
  }
}

TEST(TileKeyUtilsTest, GeoRectangleToTileKeysBoundaries) {
  const HalfQuadTreeEquirectangularTilingScheme tilingScheme;
  const GeoCoordinates southWest(GeoCoordinates::FromDegrees(-90.0, -180.0));
  const GeoCoordinates northWest(GeoCoordinates::FromDegrees(90.0, -180.0));
  const GeoCoordinates southEast(GeoCoordinates::FromDegrees(-90.0, 180.0));
  const GeoCoordinates northEast(GeoCoordinates::FromDegrees(90.0, 180.0));
  const double halfSize = 0.00005;

  const GeoRectangle southWestArea(
      GeoCoordinates(southWest.GetLatitude() - halfSize,
                     southWest.GetLongitude() - halfSize),
      GeoCoordinates(southWest.GetLatitude() + halfSize,
                     southWest.GetLongitude() + halfSize));
  const GeoRectangle southEastArea(
      GeoCoordinates(southEast.GetLatitude() - halfSize,
                     southEast.GetLongitude() - halfSize),
      GeoCoordinates(southEast.GetLatitude() + halfSize,
                     southEast.GetLongitude() + halfSize));
  const GeoRectangle northWestArea(
      GeoCoordinates(northWest.GetLatitude() - halfSize,
                     northWest.GetLongitude() - halfSize),
      GeoCoordinates(northWest.GetLatitude() + halfSize,
                     northWest.GetLongitude() + halfSize));
  const GeoRectangle northEastArea(
      GeoCoordinates(northEast.GetLatitude() - halfSize,
                     northEast.GetLongitude() - halfSize),
      GeoCoordinates(northEast.GetLatitude() + halfSize,
                     northEast.GetLongitude() + halfSize));

  const auto& levelSize = tilingScheme.GetSubdivisionScheme().GetLevelSize(15);
  const unsigned cx = levelSize.Width();
  const unsigned cy = levelSize.Height();

  {
    std::vector<TileKey> tileKeys =
        TileKeyUtils::GeoRectangleToTileKeys(tilingScheme, southWestArea, 15);
    EXPECT_EQ(2U, tileKeys.size());
    EXPECT_NE(tileKeys.end(), std::find(tileKeys.begin(), tileKeys.end(),
                                        TileKey::FromRowColumnLevel(0, 0, 15)));
    EXPECT_NE(tileKeys.end(),
              std::find(tileKeys.begin(), tileKeys.end(),
                        TileKey::FromRowColumnLevel(0, cx - 1, 15)));
  }

  {
    std::vector<TileKey> tileKeys =
        TileKeyUtils::GeoRectangleToTileKeys(tilingScheme, southEastArea, 15);
    EXPECT_EQ(2U, tileKeys.size());
    EXPECT_NE(tileKeys.end(), std::find(tileKeys.begin(), tileKeys.end(),
                                        TileKey::FromRowColumnLevel(0, 0, 15)));
    EXPECT_NE(tileKeys.end(),
              std::find(tileKeys.begin(), tileKeys.end(),
                        TileKey::FromRowColumnLevel(0, cx - 1, 15)));
  }

  {
    std::vector<TileKey> tileKeys =
        TileKeyUtils::GeoRectangleToTileKeys(tilingScheme, northWestArea, 15);
    EXPECT_EQ(2U, tileKeys.size());
    EXPECT_NE(tileKeys.end(),
              std::find(tileKeys.begin(), tileKeys.end(),
                        TileKey::FromRowColumnLevel(cy - 1, 0, 15)));
    EXPECT_NE(tileKeys.end(),
              std::find(tileKeys.begin(), tileKeys.end(),
                        TileKey::FromRowColumnLevel(cy - 1, cx - 1, 15)));
  }

  {
    std::vector<TileKey> tileKeys =
        TileKeyUtils::GeoRectangleToTileKeys(tilingScheme, northEastArea, 15);
    EXPECT_EQ(2U, tileKeys.size());
    EXPECT_NE(tileKeys.end(),
              std::find(tileKeys.begin(), tileKeys.end(),
                        TileKey::FromRowColumnLevel(cy - 1, 0, 15)));
    EXPECT_NE(tileKeys.end(),
              std::find(tileKeys.begin(), tileKeys.end(),
                        TileKey::FromRowColumnLevel(cy - 1, cx - 1, 15)));
  }
}

struct TileKeyUtilsSubTileTest {
  TileKey parentTileKey;
  TileKey relativeSubtileKey;
  TileKey absoluteSubtileKey;
};

class TileKeyUtilTest : public testing::TestWithParam<TileKeyUtilsSubTileTest> {
};

static const std::vector<TileKeyUtilsSubTileTest> tileKeys{
    {TileKey::FromRowColumnLevel(0, 0, 0), TileKey::FromRowColumnLevel(0, 0, 0),
     TileKey::FromRowColumnLevel(0, 0, 0)},
    {TileKey::FromRowColumnLevel(1, 1, 1), TileKey::FromRowColumnLevel(0, 0, 0),
     TileKey::FromRowColumnLevel(1, 1, 1)},
    {TileKey::FromRowColumnLevel(1, 1, 1), TileKey::FromRowColumnLevel(8, 8, 5),
     TileKey::FromRowColumnLevel(40, 40, 6)},
};

INSTANTIATE_TEST_SUITE_P(, TileKeyUtilTest, testing::ValuesIn(tileKeys));

TEST_P(TileKeyUtilTest, GetAbsoluteSubTileKey) {
  TileKeyUtilsSubTileTest param = GetParam();
  ASSERT_EQ(param.absoluteSubtileKey,
            TileKeyUtils::GetAbsoluteSubTileKey(param.parentTileKey,
                                                param.relativeSubtileKey));
}

TEST_P(TileKeyUtilTest, GetRelativeSubTileKey) {
  TileKeyUtilsSubTileTest param = GetParam();
  ASSERT_EQ(param.relativeSubtileKey,
            TileKeyUtils::GetRelativeSubTileKey(param.absoluteSubtileKey,
                                                param.parentTileKey.Level()));
}

}  // namespace geo
}  // namespace olp
