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

TEST(PrefetchRepositoryTest, SplitTreeLevel5) {
  repository::RootTilesForRequest root_tiles_depth;
  auto it =
      root_tiles_depth.emplace(olp::geo::TileKey::FromHereTile("5904591"), 5);

  PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first);
  ASSERT_EQ(it.first->second, 0);
  ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 1));
}

TEST(PrefetchRepositoryTest, SplitTreeLevel8) {
  repository::RootTilesForRequest root_tiles_depth;
  auto it =
      root_tiles_depth.emplace(olp::geo::TileKey::FromHereTile("5904591"), 8);

  PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first);
  ASSERT_EQ(it.first->second, 3);
  ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 4));
}

TEST(PrefetchRepositoryTest, SplitTreeLevel9) {
  repository::RootTilesForRequest root_tiles_depth;
  auto it =
      root_tiles_depth.emplace(olp::geo::TileKey::FromHereTile("5904591"), 9);

  PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first);
  ASSERT_EQ(it.first->second, 4);
  ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 5));
}

TEST(PrefetchRepositoryTest, SplitTreeLevel10) {
  repository::RootTilesForRequest root_tiles_depth;
  auto it =
      root_tiles_depth.emplace(olp::geo::TileKey::FromHereTile("5904591"), 10);

  PrefetchRepositoryTestable::SplitSubtree(root_tiles_depth, it.first);
  ASSERT_EQ(it.first->second, 0);
  ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 1) + pow(4, 6));
}

TEST(PrefetchRepositoryTest, GetSlicedTiles) {
  auto tile = olp::geo::TileKey::FromHereTile("5904591");
  auto root_tiles_depth =
      PrefetchRepositoryTestable::GetSlicedTiles({tile}, 0, 0);
  ASSERT_EQ(root_tiles_depth.size(), 1);
  auto parent = tile.ChangedLevelBy(-4);
  ASSERT_EQ(root_tiles_depth.begin()->first, parent);
  ASSERT_EQ(root_tiles_depth.begin()->second, 4);
}

TEST(PrefetchRepositoryTest, GetSlicedTilesWithLevelsSpecified) {
  auto tile = olp::geo::TileKey::FromHereTile("5904591");
  auto root_tiles_depth =
      PrefetchRepositoryTestable::GetSlicedTiles({tile}, 11, 13);
  ASSERT_EQ(root_tiles_depth.begin()->first, tile);
  ASSERT_EQ(root_tiles_depth.begin()->second, 2);
}

TEST(PrefetchRepositoryTest, GetSlicedTilesWithLevelsSpecified2) {
  auto tile = olp::geo::TileKey::FromHereTile("5904591");  // level 11
  auto root_tiles_depth =
      PrefetchRepositoryTestable::GetSlicedTiles({tile}, 1, 13);
  auto parent = tile.ChangedLevelBy(-10);
  ASSERT_EQ(root_tiles_depth.find(parent)->second, 2);
  ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 3) + pow(4, 8));
}

TEST(PrefetchRepositoryTest, GetSlicedTilesDifferentLevelsButLevelsSpecified) {
  auto tile1 = olp::geo::TileKey::FromHereTile("5904591");  // level 11
  auto tile2 = olp::geo::TileKey::FromHereTile(
      "23618365");  // level 12, this tile will be in the same subtree
  auto root_tiles_depth =
      PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 1, 13);
  auto parent = tile1.ChangedLevelBy(-10);
  ASSERT_EQ(root_tiles_depth.find(parent)->second, 2);
  ASSERT_EQ(root_tiles_depth.size(), 1 + pow(4, 3) + pow(4, 8));
}

TEST(PrefetchRepositoryTest, GetSlicedTilesSibilings) {
  auto tile1 = olp::geo::TileKey::FromHereTile("23618366");
  auto tile2 = olp::geo::TileKey::FromHereTile("23618365");
  auto root_tiles_depth =
      PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 0, 0);
  ASSERT_EQ(root_tiles_depth.size(), 1);
  auto parent1 = tile1.ChangedLevelBy(-4);
  auto parent2 = tile1.ChangedLevelBy(-4);
  ASSERT_EQ(parent1, parent2);
  ASSERT_EQ(root_tiles_depth.begin()->first, parent1);
  ASSERT_EQ(root_tiles_depth.begin()->second, 4);
}

TEST(PrefetchRepositoryTest, GetSlicedTilesSibilingsWithLevelsSpecified) {
  auto tile1 = olp::geo::TileKey::FromHereTile("23618366");
  auto tile2 = olp::geo::TileKey::FromHereTile("23618365");
  auto root_tiles_depth =
      PrefetchRepositoryTestable::GetSlicedTiles({tile1, tile2}, 11, 12);
  ASSERT_EQ(root_tiles_depth.size(), 1);
  auto parent1 = tile1.ChangedLevelBy(-1);
  auto parent2 = tile1.ChangedLevelBy(-1);
  ASSERT_EQ(parent1, parent2);
  ASSERT_EQ(root_tiles_depth.begin()->first, parent1);
  ASSERT_EQ(root_tiles_depth.begin()->second, 1);
}
}  // namespace
