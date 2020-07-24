/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <repositories/PrefetchTilesRepository.h>

namespace {
namespace repository = olp::dataservice::read::repository;

class PrefetchRepositoryTestable
    : protected repository::PrefetchTilesRepository {
 public:
  using repository::PrefetchTilesRepository::GetSlicedTiles;
  using repository::PrefetchTilesRepository::SplitSubtree;
};

TEST(PrefetchRepositoryTest, SplitTreeLevels) {
  {
    SCOPED_TRACE("Split with depth 5");
    repository::RootTilesForRequest root_tiles_depth;
    auto tile = olp::geo::TileKey::FromHereTile("5904591");
    auto it = root_tiles_depth.emplace(tile, 5);

    PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first, tile,
                                             0);
    ASSERT_EQ(it.first->second, 0);
    ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 1));
  }
  {
    SCOPED_TRACE("Split with depth 8");
    repository::RootTilesForRequest root_tiles_depth;
    auto tile = olp::geo::TileKey::FromHereTile("5904591");
    auto it = root_tiles_depth.emplace(tile, 8);

    PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first, tile,
                                             0);
    ASSERT_EQ(it.first->second, 3);
    ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 4));
  }
  {
    SCOPED_TRACE("Split with depth 9");
    repository::RootTilesForRequest root_tiles_depth;
    auto tile = olp::geo::TileKey::FromHereTile("5904591");
    auto it = root_tiles_depth.emplace(tile, 9);

    PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first, tile,
                                             0);
    ASSERT_EQ(it.first->second, 4);
    ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 5));
  }
  {
    SCOPED_TRACE("Split with depth 10");
    repository::RootTilesForRequest root_tiles_depth;
    auto tile = olp::geo::TileKey::FromHereTile("5904591");
    auto it = root_tiles_depth.emplace(tile, 10);

    PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first, tile,
                                             0);
    ASSERT_EQ(it.first->second, 0);
    ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 1) + pow(4, 6));
  }
}
TEST(PrefetchRepositoryTest, SplitTreeLevelMinLevelSet) {
  repository::RootTilesForRequest root_tiles_depth;
  auto tile = olp::geo::TileKey::FromHereTile("5904591");
  auto it = root_tiles_depth.emplace(tile, 10);

  // want to slice up starting from level 13
  PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first, tile,
                                           13);
  ASSERT_EQ(root_tiles_depth.find(tile), root_tiles_depth.end());
  ASSERT_EQ(root_tiles_depth.size(), pow(4, 1) + pow(4, 6));
}

TEST(PrefetchRepositoryTest, GetSlicedTilesNoLevels) {
  auto tile = olp::geo::TileKey::FromHereTile("5904591");
  auto root_tiles_depth = PrefetchRepositoryTestable::GetSlicedTiles(
      {tile}, olp::geo::TileKey().Level(), olp::geo::TileKey().Level());
  ASSERT_EQ(root_tiles_depth.size(), 1);
  auto parent = tile.ChangedLevelBy(-4);
  ASSERT_EQ(root_tiles_depth.size(), 1);
  ASSERT_EQ(root_tiles_depth.begin()->first, parent);
  ASSERT_EQ(root_tiles_depth.begin()->second, 4);
}

TEST(PrefetchRepositoryTest, GetSlicedTilesWithLevelsSpecified) {
  auto tile = olp::geo::TileKey::FromHereTile("5904591");  // level 11
  {
    SCOPED_TRACE(
        "Get sliced tiles with min level equal requested root tile level");
    auto root_tiles_depth =
        PrefetchRepositoryTestable::GetSlicedTiles({tile}, 11, 13);
    auto parent = tile.ChangedLevelTo(13 - 4);  // max_level -4
    ASSERT_EQ(root_tiles_depth.size(), 1);
    ASSERT_EQ(root_tiles_depth.begin()->first, parent);
    ASSERT_EQ(root_tiles_depth.begin()->second, 4);
  }
  {
    SCOPED_TRACE(
        "Get sliced tiles with min level smaller than requested root tile "
        "level");
    auto root_tiles_depth =
        PrefetchRepositoryTestable::GetSlicedTiles({tile}, 1, 13);
    auto parent = tile.ChangedLevelTo(5);
    // sliced levels should be 0, 5, 10
    ASSERT_EQ(root_tiles_depth.find(parent)->second, 4);
    // 1 tile on each level
    ASSERT_EQ(root_tiles_depth.size(), 3);
  }
  {
    SCOPED_TRACE(
        "Get sliced tiles with min level greater than requested root tile "
        "level");
    auto root_tiles_depth =
        PrefetchRepositoryTestable::GetSlicedTiles({tile}, 14, 16);
    // sliced levels should be 12
    auto parent = tile.ChangedLevelTo(12);
    ASSERT_EQ(root_tiles_depth.find(parent)->second, 4);
    //  and 4^(12-11) tiles on level 12
    ASSERT_EQ(root_tiles_depth.size(), 4);
  }
}

TEST(PrefetchRepositoryTest, GetSlicedTilesMultipleRootTiles) {
  auto tile1 = olp::geo::TileKey::FromHereTile("5904591");  // level 11
  auto tile2 = olp::geo::TileKey::FromHereTile(
      "23618365");  // level 12, this tile will be in the same subtree
  {
    SCOPED_TRACE(
        "Get sliced tiles with min level smaller than both requested root tile "
        "levels");
    auto root_tiles_depth =
        PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 1, 13);
    // sliced levels should be 0, 5, 10
    auto parent = tile1.ChangedLevelTo(5);
    ASSERT_EQ(root_tiles_depth.find(parent)->second, 4);
    ASSERT_EQ(root_tiles_depth.size(), 3);
  }
  {
    SCOPED_TRACE("Get sliced tiles with min/max levels is zero");
    auto root_tiles_depth =
        PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 0, 0);
    // sliced levels should be 0
    auto parent = tile1.ChangedLevelTo(0);
    ASSERT_EQ(root_tiles_depth.find(parent)->second, 4);
    ASSERT_EQ(root_tiles_depth.size(), 1);
  }
  {
    SCOPED_TRACE(
        "Get sliced tiles with min level greter than first root tile level, "
        "but equal second");

    auto root_tiles_depth =
        PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 12, 13);
    // sliced levels is 9
    auto parent = tile1.ChangedLevelTo(13 - 4);
    ASSERT_EQ(root_tiles_depth.find(parent)->second, 4);
    // no duplicates for sliced tiles, as tile1 is parent tile2
    ASSERT_EQ(root_tiles_depth.size(), 1);
  }
  {
    SCOPED_TRACE(
        "Get sliced tiles with min level greter than both requested root tiles "
        "levels");
    auto root_tiles_depth =
        PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 15, 16);
    // sliced levels is 7 and 12, as tile1 on level 11, this means we need to
    // get 4 tiles on level 12, and so on, so on level 15 it is and 4^(15-11)
    // tiles the best way to query for each of 4 tiles on level 12
    auto parent = tile1.ChangedLevelTo(16 - 4);
    ASSERT_EQ(root_tiles_depth.find(parent)->second, 4);
    // no duplicates for sliced tiles, as tile1 is parent tile2
    ASSERT_EQ(root_tiles_depth.size(), 4);
  }
}

TEST(PrefetchRepositoryTest, GetSlicedTilesSiblingsNoLevels) {
  auto tile1 = olp::geo::TileKey::FromHereTile("23618366");
  auto tile2 = olp::geo::TileKey::FromHereTile("23618365");
  auto root_tiles_depth = PrefetchRepositoryTestable::GetSlicedTiles(
      {tile1, tile2}, olp::geo::TileKey().Level(), olp::geo::TileKey().Level());
  ASSERT_EQ(root_tiles_depth.size(), 1);
  auto parent1 = tile1.ChangedLevelBy(-4);
  auto parent2 = tile1.ChangedLevelBy(-4);
  ASSERT_EQ(parent1, parent2);
  ASSERT_EQ(root_tiles_depth.begin()->first, parent1);
  ASSERT_EQ(root_tiles_depth.begin()->second, 4);
}

TEST(PrefetchRepositoryTest, GetSlicedTilesSiblings) {
  auto tile1 = olp::geo::TileKey::FromHereTile("23618366");
  auto tile2 = olp::geo::TileKey::FromHereTile("23618365");
  auto root_tiles_depth =
      PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 11, 12);
  ASSERT_EQ(root_tiles_depth.size(), 1);
  auto parent1 = tile1.ChangedLevelTo(12 - 4);
  auto parent2 = tile1.ChangedLevelTo(12 - 4);
  ASSERT_EQ(parent1, parent2);
  ASSERT_EQ(root_tiles_depth.begin()->first, parent1);
  ASSERT_EQ(root_tiles_depth.begin()->second, 4);
}
}  // namespace
