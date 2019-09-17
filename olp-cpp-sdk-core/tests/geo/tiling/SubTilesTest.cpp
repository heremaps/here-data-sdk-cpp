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

#include <olp/core/geo/tiling/SubTiles.h>
#include <olp/core/geo/tiling/TileKey.h>

#include <gtest/gtest.h>

#include <queue>
#include <vector>

namespace olp {
namespace geo {
TEST(SubTilesTest, Constructor) {
  const TileKey tileKey = TileKey::FromRowColumnLevel(738, 456, 10);

  const SubTiles childrenTiles(tileKey);
  for (auto childTile : childrenTiles) {
    EXPECT_EQ(tileKey, childTile.Parent());

    const SubTiles grandchildrenTiles(childTile);
    for (auto grandchildTile : grandchildrenTiles) {
      EXPECT_EQ(childTile, grandchildTile.Parent());
      EXPECT_EQ(tileKey, grandchildTile.ChangedLevelBy(-2));
    }
  }
}

std::uint32_t EnumerateSubTree(const TileKey& tileKey, std::uint32_t maxLevel) {
  std::uint32_t count = 0;

  std::queue<TileKey> tiles;
  tiles.push(tileKey);

  while (!tiles.empty()) {
    const TileKey thisTileKey = tiles.front();
    tiles.pop();

    if (thisTileKey.Level() > maxLevel) {
      continue;
    }

    count++;

    if (thisTileKey.Level() == maxLevel) {
      continue;
    }

    const SubTiles subTiles(thisTileKey);

    for (auto subTileKey : subTiles) {
      tiles.push(subTileKey);
    }
  }

  return count;
}

TEST(SubTilesTest, QuadTreeSize) {
  const TileKey root = TileKey::FromRowColumnLevel(0, 0, 0);

  const std::uint32_t minLevel = root.Level();
  const std::uint32_t maxLevel = minLevel + 5;

  std::uint32_t quadTreeTileCount = EnumerateSubTree(root, maxLevel);

  std::uint32_t quadTreeExpectedTileCount = 0;

  for (std::uint32_t level = minLevel; level <= maxLevel; ++level) {
    quadTreeExpectedTileCount += 1 << ((level - minLevel) << 1);
  }

  EXPECT_EQ(quadTreeExpectedTileCount, quadTreeTileCount);
}

TEST(SubTilesTest, Size) {
  const TileKey root = TileKey::FromRowColumnLevel(0, 0, 0);

  EXPECT_EQ(4U, SubTiles(root, 1).Size());
  EXPECT_EQ(16U, SubTiles(root, 2).Size());
  EXPECT_EQ(64U, SubTiles(root, 3).Size());
  EXPECT_EQ(256U, SubTiles(root, 4).Size());
}

TEST(SubTilesTest, Masking) {
  const TileKey root = TileKey::FromRowColumnLevel(0, 0, 0);

  {
    const SubTiles subTiles(root, 2, 0xF0F0);
    std::vector<TileKey> subTileKeys;
    std::copy(subTiles.cbegin(), subTiles.cend(),
              std::back_inserter(subTileKeys));
    EXPECT_EQ(subTiles.Size(), subTileKeys.size() * 2);
  }

  {
    const SubTiles subTiles(root, 3, 0xF0F0);
    std::vector<TileKey> subTileKeys;
    std::copy(subTiles.cbegin(), subTiles.cend(),
              std::back_inserter(subTileKeys));
    EXPECT_EQ(subTiles.Size(), subTileKeys.size() * 2);
  }
}
}  // namespace geo
}  // namespace olp
