/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <vector>

#include <gmock/gmock.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>

using namespace ::testing;
using namespace olp::geo;
using namespace olp::dataservice::read;
using TileKeys = std::vector<TileKey>;

namespace {

TEST(PrefetchTilesRequestTest, TileKeys) {
  {
    SCOPED_TRACE("lvalue test");

    PrefetchTilesRequest request;
    const TileKeys expected_tiles = {TileKey::FromHereTile("1234"),
                                     TileKey::FromHereTile("12345")};

    request.WithTileKeys(expected_tiles);

    EXPECT_EQ(expected_tiles, request.GetTileKeys());
  }
  {
    SCOPED_TRACE("rvalue test");

    PrefetchTilesRequest request;
    const TileKeys expected_tiles = {TileKey::FromHereTile("1234"),
                                     TileKey::FromHereTile("12345")};
    const TileKeys expected_tiles2 = {TileKey::FromHereTile("12346"),
                                      TileKey::FromHereTile("123456")};

    auto tile_keys = expected_tiles;
    request.WithTileKeys(std::move(tile_keys));

    EXPECT_EQ(expected_tiles, request.GetTileKeys());

    request.WithTileKeys(
        {TileKey::FromHereTile("12346"), TileKey::FromHereTile("123456")});

    EXPECT_EQ(expected_tiles2, request.GetTileKeys());
  }
}

}  // namespace
