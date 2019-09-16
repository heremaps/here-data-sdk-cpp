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
#include <iostream>

#include <olp/core/geo/tiling/TileKey.h>

using namespace olp::geo;

void printBits(std::uint64_t v) {
  unsigned int n = sizeof(v) * 8;
  for (unsigned int i = 0; i < n; i++) {
    std::cout << ((v >> (n - i - 1)) & 1);
  }
  std::cout << " = " << v << std::endl;
}

TEST(TileKeyTest, Init) {}

TEST(TileKeyTest, Valid) {
  TileKey quad1;
  ASSERT_FALSE(quad1.IsValid());

  TileKey quad2 = TileKey::FromRowColumnLevel(1, 2, 3);
  ASSERT_TRUE(quad2.IsValid());
}

TEST(TileKeyTest, RowColumnLevel) {
  TileKey quad = TileKey::FromRowColumnLevel(1, 2, 3);
  ASSERT_EQ(1u, quad.Row());
  ASSERT_EQ(2u, quad.Column());
  ASSERT_EQ(3u, quad.Level());
}

TEST(TileKeyTest, Operators) {
  TileKey quad = TileKey::FromRowColumnLevel(1, 2, 3);
  TileKey quad2 = TileKey::FromRowColumnLevel(1, 2, 4);
  ASSERT_TRUE(quad != quad2);
  ASSERT_FALSE(quad == quad2);
  quad2 = quad;
  ASSERT_TRUE(quad == quad2);
  ASSERT_FALSE(quad != quad2);

  // test quads on different levels
  ASSERT_TRUE(TileKey::FromRowColumnLevel(1, 2, 3) <
              TileKey::FromRowColumnLevel(1, 2, 4));
  ASSERT_FALSE(TileKey::FromRowColumnLevel(1, 2, 4) <
               TileKey::FromRowColumnLevel(1, 2, 3));

  // test quads on same level
  ASSERT_TRUE(TileKey::FromRowColumnLevel(0, 0, 1) <
              TileKey::FromRowColumnLevel(1, 0, 1));
  ASSERT_FALSE(TileKey::FromRowColumnLevel(1, 0, 1) <
               TileKey::FromRowColumnLevel(0, 0, 1));

  // identical quads must not be smaller
  ASSERT_FALSE(TileKey::FromRowColumnLevel(1, 1, 1) <
               TileKey::FromRowColumnLevel(1, 1, 1));

  std::hash<TileKey> quadHasher;
  ASSERT_EQ(quadHasher(TileKey::FromRowColumnLevel(1, 1, 1)),
            quadHasher(TileKey::FromRowColumnLevel(1, 1, 1)));
  ASSERT_NE(quadHasher(TileKey::FromRowColumnLevel(1, 1, 1)),
            quadHasher(TileKey::FromRowColumnLevel(1, 1, 2)));
}

TEST(TileKeyTest, QuadKeys) {
  TileKey quad;
  quad = TileKey::FromRowColumnLevel(0, 0, 0);
  ASSERT_EQ("-", quad.ToQuadKey());
  ASSERT_EQ(1u, quad.ToQuadKey64());
  quad = TileKey::FromRowColumnLevel(1, 1, 1);
  ASSERT_EQ("3", quad.ToQuadKey());
  ASSERT_EQ(7u, quad.ToQuadKey64());

  quad = TileKey::FromRowColumnLevel(3, 5, 3);
  ASSERT_EQ("123", quad.ToQuadKey());
  TileKey quad2 = TileKey::FromQuadKey(quad.ToQuadKey());
  ASSERT_EQ(quad, quad2);

  TileKey world = TileKey::FromQuadKey("-");
  ASSERT_EQ(0u, world.Level());
  ASSERT_EQ(0u, world.Row());
  ASSERT_EQ(0u, world.Column());

  TileKey invalid = TileKey::FromQuadKey("");
  ASSERT_FALSE(invalid.IsValid());
}

TEST(TileKeyTest, HereTiles) {
  TileKey quad;
  quad = TileKey::FromRowColumnLevel(0, 0, 0);
  ASSERT_EQ("1", quad.ToHereTile());
  ASSERT_EQ(1u, quad.ToQuadKey64());
  quad = TileKey::FromRowColumnLevel(1, 1, 1);
  ASSERT_EQ("7", quad.ToHereTile());
  ASSERT_EQ(7u, quad.ToQuadKey64());

  quad = TileKey::FromRowColumnLevel(3, 5, 3);
  ASSERT_EQ("123", quad.ToQuadKey());
  ASSERT_EQ("91", quad.ToHereTile());
  TileKey quad2 = TileKey::FromHereTile(quad.ToHereTile());
  ASSERT_EQ(quad, quad2);

  TileKey world = TileKey::FromHereTile("1");
  ASSERT_EQ(0u, world.Level());
  ASSERT_EQ(0u, world.Row());
  ASSERT_EQ(0u, world.Column());

  quad = TileKey::FromHereTile("91");
  ASSERT_EQ(3u, quad.Level());
  ASSERT_EQ(3u, quad.Row());
  ASSERT_EQ(5u, quad.Column());

  TileKey invalid = TileKey::FromHereTile("");
  ASSERT_FALSE(invalid.IsValid());
}

TEST(TileKeyTest, MoveToLevel) {
  TileKey quad = TileKey::FromRowColumnLevel(0, 0, 5);
  ASSERT_EQ(quad.ChangedLevelBy(-2), quad.ChangedLevelTo(3));
  ASSERT_EQ(quad.ChangedLevelBy(2), quad.ChangedLevelTo(7));
}

TEST(TileKeyTest, ChangeLevel) {
  TileKey quad = TileKey::FromRowColumnLevel(2, 3, 2);
  TileKey quad2 = quad;
  quad = quad.ChangedLevelBy(0);
  ASSERT_EQ(quad, quad2);
  quad = quad.ChangedLevelBy(1);
  ASSERT_EQ(TileKey::FromRowColumnLevel(4, 6, 3), quad);
  quad = quad.ChangedLevelBy(-1);
  ASSERT_EQ(quad2.ToQuadKey(), quad.ToQuadKey());
}

TEST(TileKeyTest, Parent) {
  TileKey quad = TileKey::FromRowColumnLevel(3, 3, 2);
  ASSERT_EQ(quad.Parent(), TileKey::FromRowColumnLevel(1, 1, 1));
  ASSERT_EQ(quad.Parent().Parent(), TileKey::FromRowColumnLevel(0, 0, 0));
  ASSERT_FALSE(quad.Parent().Parent().Parent().IsValid());
}

TEST(TileKeyTest, QuadKeyGetSubkey) {
  ASSERT_EQ(4u, TileKey::FromRowColumnLevel(2, 2, 2).GetSubkey64(1));
  ASSERT_EQ(5u, TileKey::FromRowColumnLevel(2, 3, 2).GetSubkey64(1));
  ASSERT_EQ(6u, TileKey::FromRowColumnLevel(3, 2, 2).GetSubkey64(1));
  ASSERT_EQ(7u, TileKey::FromRowColumnLevel(3, 3, 2).GetSubkey64(1));

  ASSERT_EQ(16u,
            QuadKey64Helper{TileKey::FromRowColumnLevel(4, 4, 3).ToQuadKey64()}
                .GetSubkey(2)
                .key);
  ASSERT_EQ(17u,
            QuadKey64Helper{TileKey::FromRowColumnLevel(4, 5, 3).ToQuadKey64()}
                .GetSubkey(2)
                .key);
  ASSERT_EQ(18u,
            QuadKey64Helper{TileKey::FromRowColumnLevel(5, 4, 3).ToQuadKey64()}
                .GetSubkey(2)
                .key);
  ASSERT_EQ(19u,
            QuadKey64Helper{TileKey::FromRowColumnLevel(5, 5, 3).ToQuadKey64()}
                .GetSubkey(2)
                .key);

  ASSERT_EQ(1u, TileKey::FromRowColumnLevel(4, 4, 3).GetSubkey64(0));
  ASSERT_EQ(1u, TileKey::FromRowColumnLevel(0, 0, 0).GetSubkey64(0));
}

TEST(TileKeyTest, QuadKeyAddSubkey) {
  ASSERT_EQ(TileKey::FromRowColumnLevel(2, 2, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey64(4));
  ASSERT_EQ(TileKey::FromRowColumnLevel(2, 3, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey64(5));
  ASSERT_EQ(TileKey::FromRowColumnLevel(3, 2, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey64(6));
  ASSERT_EQ(TileKey::FromRowColumnLevel(3, 3, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey64(7));

  ASSERT_EQ(TileKey::FromRowColumnLevel(2, 2, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("0"));
  ASSERT_EQ(TileKey::FromRowColumnLevel(2, 3, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("1"));
  ASSERT_EQ(TileKey::FromRowColumnLevel(3, 2, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("2"));
  ASSERT_EQ(TileKey::FromRowColumnLevel(3, 3, 2),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("3"));

  ASSERT_EQ(TileKey::FromRowColumnLevel(4, 4, 3).ToQuadKey64(),
            QuadKey64Helper{TileKey::FromRowColumnLevel(1, 1, 1).ToQuadKey64()}
                .AddedSubkey(QuadKey64Helper{16})
                .key);
  ASSERT_EQ(TileKey::FromRowColumnLevel(4, 5, 3).ToQuadKey64(),
            QuadKey64Helper{TileKey::FromRowColumnLevel(1, 1, 1).ToQuadKey64()}
                .AddedSubkey(QuadKey64Helper{17})
                .key);
  ASSERT_EQ(TileKey::FromRowColumnLevel(5, 4, 3).ToQuadKey64(),
            QuadKey64Helper{TileKey::FromRowColumnLevel(1, 1, 1).ToQuadKey64()}
                .AddedSubkey(QuadKey64Helper{18})
                .key);
  ASSERT_EQ(TileKey::FromRowColumnLevel(5, 5, 3).ToQuadKey64(),
            QuadKey64Helper{TileKey::FromRowColumnLevel(1, 1, 1).ToQuadKey64()}
                .AddedSubkey(QuadKey64Helper{19})
                .key);

  ASSERT_EQ(TileKey::FromRowColumnLevel(4, 4, 3),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("00"));
  ASSERT_EQ(TileKey::FromRowColumnLevel(4, 5, 3),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("01"));
  ASSERT_EQ(TileKey::FromRowColumnLevel(5, 4, 3),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("02"));
  ASSERT_EQ(TileKey::FromRowColumnLevel(5, 5, 3),
            TileKey::FromRowColumnLevel(1, 1, 1).AddedSubkey("03"));

  ASSERT_EQ(TileKey::FromRowColumnLevel(4, 4, 3),
            TileKey::FromRowColumnLevel(4, 4, 3).AddedSubkey(""));

  ASSERT_EQ(TileKey::FromRowColumnLevel(4, 4, 3),
            TileKey::FromRowColumnLevel(4, 4, 3).AddedSubkey64(1));
  ASSERT_EQ(TileKey::FromRowColumnLevel(0, 0, 0),
            TileKey::FromRowColumnLevel(0, 0, 0).AddedSubkey64(1));
}

TEST(TileKeyTest, QuadKey64Helper) {
  TileKey quad = TileKey::FromRowColumnLevel(2, 2, 2);
  QuadKey64Helper helper{quad.ToQuadKey64()};
  ASSERT_EQ(quad.Parent().ToQuadKey64(), helper.Parent().key);
}

TEST(TileKeyTest, GetChild) {
  //           90
  //      -----------
  //      | 10 | 11 |
  // -180 ----------- 180
  //      | 00 | 01 |
  //      -----------
  //         -90

  TileKey quad = TileKey::FromRowColumnLevel(0, 0, 0);
  ASSERT_EQ(quad.GetChild(0), TileKey::FromRowColumnLevel(0, 0, 1));
  ASSERT_EQ(quad.GetChild(1), TileKey::FromRowColumnLevel(0, 1, 1));
  ASSERT_EQ(quad.GetChild(2), TileKey::FromRowColumnLevel(1, 0, 1));
  ASSERT_EQ(quad.GetChild(3), TileKey::FromRowColumnLevel(1, 1, 1));
  ASSERT_EQ(quad.GetChild(TileKey::TileKeyQuadrant::SW),
            TileKey::FromRowColumnLevel(0, 0, 1));
  ASSERT_EQ(quad.GetChild(TileKey::TileKeyQuadrant::SE),
            TileKey::FromRowColumnLevel(0, 1, 1));
  ASSERT_EQ(quad.GetChild(TileKey::TileKeyQuadrant::NW),
            TileKey::FromRowColumnLevel(1, 0, 1));
  ASSERT_EQ(quad.GetChild(TileKey::TileKeyQuadrant::NE),
            TileKey::FromRowColumnLevel(1, 1, 1));
}

TEST(TileKeyTest, RelationshipToParent) {
  TileKey sw = TileKey::FromRowColumnLevel(0, 0, 1);
  ASSERT_EQ(sw.RelationshipToParent(), TileKey::TileKeyQuadrant::SW);
  TileKey se = TileKey::FromRowColumnLevel(0, 1, 1);
  ASSERT_EQ(se.RelationshipToParent(), TileKey::TileKeyQuadrant::SE);
  TileKey nw = TileKey::FromRowColumnLevel(1, 0, 1);
  ASSERT_EQ(nw.RelationshipToParent(), TileKey::TileKeyQuadrant::NW);
  TileKey ne = TileKey::FromRowColumnLevel(1, 1, 1);
  ASSERT_EQ(ne.RelationshipToParent(), TileKey::TileKeyQuadrant::NE);
  TileKey root = TileKey::FromRowColumnLevel(0, 0, 0);
  ASSERT_EQ(root.RelationshipToParent(), TileKey::TileKeyQuadrant::Invalid);
}

TEST(TileKeyTest, ParentChildRelationIsIrreflexive) {
  const auto parent = TileKey::FromRowColumnLevel(0, 0, 0);
  EXPECT_FALSE(parent.IsChildOf(parent));
  EXPECT_FALSE(parent.IsParentOf(parent));
}

TEST(TileKeyTest, ParentChildRelation) {
  const auto parent = TileKey::FromRowColumnLevel(0, 0, 0);

  for (std::uint8_t childIndex = 0; childIndex < 4; childIndex++) {
    const auto child = parent.GetChild(childIndex);
    EXPECT_TRUE(child.IsChildOf(parent));
    EXPECT_TRUE(parent.IsParentOf(child));
  }
}

TEST(TileKeyTest, ParentChildRelationIsTransitive) {
  const auto parent = TileKey::FromRowColumnLevel(0, 0, 0);
  const auto child = parent.GetChild(0);

  for (std::uint8_t grandChildIndex = 0; grandChildIndex < 4;
       grandChildIndex++) {
    const auto grandChild = child.GetChild(grandChildIndex);
    EXPECT_TRUE(grandChild.IsChildOf(parent));
    EXPECT_TRUE(parent.IsParentOf(grandChild));
  }
}

TEST(TileKeyTest, GetNearestAvailableTileKeyLevel) {
  {
    SCOPED_TRACE("Empty level set - return empty");
    TileKeyLevels levels;

    auto level = GetNearestAvailableTileKeyLevel(levels, 0);

    EXPECT_FALSE(level);
  }

  {
    SCOPED_TRACE("Reference below minimum - minimum set level is returned");
    TileKeyLevels levels{0xF0};
    const uint32_t referenceLevel = 0u;

    auto level = GetNearestAvailableTileKeyLevel(levels, referenceLevel);

    ASSERT_TRUE(level);
    EXPECT_EQ(4u, *level);
  }

  {
    SCOPED_TRACE("Reference above maximum - maximum set level is returned");
    TileKeyLevels levels{0xF0};
    const uint32_t referenceLevel = 20u;

    auto level = GetNearestAvailableTileKeyLevel(levels, referenceLevel);

    ASSERT_TRUE(level);
    EXPECT_EQ(7u, *level);
  }

  {
    SCOPED_TRACE("Reference between min and max - set level is returned");
    TileKeyLevels levels{0x0FF0};
    const uint32_t referenceLevel = 8u;

    auto level = GetNearestAvailableTileKeyLevel(levels, referenceLevel);

    ASSERT_TRUE(level);
    EXPECT_EQ(referenceLevel, *level);
  }

  {
    SCOPED_TRACE("Next level 3 above and 4 below - 3 above is returned");
    TileKeyLevels levels{std::string("100000010000")};  // 0x810
    const uint32_t referenceLevel = 8u;

    auto level = GetNearestAvailableTileKeyLevel(levels, referenceLevel);

    ASSERT_TRUE(level);
    EXPECT_EQ(11u, *level);
  }

  {
    SCOPED_TRACE("Next level 4 above and 3 below - 3 below is returned");
    TileKeyLevels levels{std::string("100000010000")};  // 0x810
    const uint32_t referenceLevel = 7u;

    auto level = GetNearestAvailableTileKeyLevel(levels, referenceLevel);

    ASSERT_TRUE(level);
    EXPECT_EQ(4u, *level);
  }

  {
    SCOPED_TRACE("Next level above and below are equal - above is returned");
    TileKeyLevels levels{std::string("10000010000")};  // 0x410
    const uint32_t referenceLevel = 7u;

    auto level = GetNearestAvailableTileKeyLevel(levels, referenceLevel);

    ASSERT_TRUE(level);
    EXPECT_EQ(10u, *level);
  }
}
