/*
 * Copyright (C) 2025 HERE Europe B.V.
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

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <olp/core/geo/tiling/PathTiling.h>

#include "olp/core/geo/tiling/TilingSchemeRegistry.h"

namespace olp {
namespace geo {

const auto kBerlin1 = GeoCoordinates::FromDegrees(52.514176, 13.339062);
const auto kBerlin2 = GeoCoordinates::FromDegrees(52.517029, 13.387142);
const auto kBerlin3 = GeoCoordinates::FromDegrees(52.490536, 13.397480);

TEST(PathTilingTest, GeoSegmentsGenerator) {
  std::vector<GeoCoordinates> coordinates{
      kBerlin1,
      kBerlin2,
      kBerlin3,
  };
  auto generator =
      MakeGeoSegmentsGenerator(coordinates.begin(), coordinates.end());
  ASSERT_TRUE(generator->Next() == std::make_tuple(kBerlin1, kBerlin2));
  ASSERT_TRUE(generator->Next() == std::make_tuple(kBerlin2, kBerlin3));
  EXPECT_FALSE(generator->Next());
}

TEST(PathTilingTest, Path) {
  std::vector<GeoCoordinates> coordinates{
      kBerlin1,
      kBerlin2,
      kBerlin3,
  };

  auto generator =
      MakeGeoSegmentsGenerator(coordinates.begin(), coordinates.end());
  auto tiling_scheme = std::make_shared<HalfQuadTreeIdentityTilingScheme>();
  auto path_generator = PathTilingGenerator(generator, tiling_scheme, 16, 1);
  std::vector<uint64_t> tile_keys;

  for (auto tile = path_generator.Next(); tile; tile = path_generator.Next()) {
    tile_keys.push_back(tile->ToQuadKey64());
  }

  EXPECT_THAT(tile_keys,
              testing::ElementsAre(
                  6046300013, 6046300015, 6046300101, 6046300024, 6046300026,
                  6046300112, 6046300025, 6046300027, 6046300113, 6046300028,
                  6046300030, 6046300116, 6046300029, 6046300031, 6046300117,
                  6046310952, 6046310954, 6046311040, 6046310953, 6046310955,
                  6046311041, 6046311042, 6046311043, 6046310958, 6046311044,
                  6046311046, 6046310959, 6046311045, 6046311047, 6046310970,
                  6046311056, 6046311058, 6046310971, 6046311057, 6046311059,
                  6046310974, 6046311060, 6046311062, 6046310937, 6046310940,
                  6046310941, 6046310939, 6046310942, 6046310943, 6046310961,
                  6046310964, 6046310965, 6046310963, 6046310966, 6046310967,
                  6046310969, 6046310972, 6046310973, 6046310962, 6046310968));
}

}  // namespace geo
}  // namespace olp
