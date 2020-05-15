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

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include "../src/repositories/QuadTreeIndex.h"
namespace {
using namespace olp;
using namespace dataservice::read;
using namespace olp::tests::common;

constexpr auto HTTP_RESPONSE_QUADKEYS_5904591 =
    R"jsonString({"subQuads": [{"version":4,"subQuadKey":"4","dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"},{"version":4,"subQuadKey":"5","dataHandle":"e119d20e-c7c6-4563-ae88-8aa5c6ca75c3"},{"version":4,"subQuadKey":"6","dataHandle":"a7a1afdf-db7e-4833-9627-d38bee6e2f81"},{"version":4,"subQuadKey":"7","dataHandle":"9d515348-afce-44e8-bc6f-3693cfbed104"},{"version":4,"subQuadKey":"1","dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": [{"version":4,"partition":"1476147","dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}]})jsonString";

TEST(QuadTreeIndexTest, ParseBlob) {
  using testing::Return;
  {
    SCOPED_TRACE("Parse json and store to blob");
    QuadTreeIndex index;
    auto tile_key = olp::geo::TileKey::FromHereTile("5904591");
    index.FromLayerIndex(
        index.FromJson(tile_key, 4, HTTP_RESPONSE_QUADKEYS_5904591));
    auto data = index.find(tile_key);
    EXPECT_EQ(data.data_handle_, "e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1");

    auto data2 = index.find(olp::geo::TileKey::FromHereTile("21395"));
    EXPECT_EQ(data2.data_handle_, "95c5c703-e00e-4c38-841e-e419367474f1");

    // 23618364  23618365 23618366 23618367
    auto data4 = index.find(olp::geo::TileKey::FromHereTile("23618364"));
    EXPECT_EQ(data4.data_handle_, "f9a9fd8e-eb1b-48e5-bfdb-4392b3826443");
    auto data5 = index.find(olp::geo::TileKey::FromHereTile("23618367"));
    EXPECT_EQ(data5.data_handle_, "9d515348-afce-44e8-bc6f-3693cfbed104");
  }
}
}  // namespace
