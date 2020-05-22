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

#include <sstream>

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include "../src/repositories/QuadTreeIndex.h"
namespace {
using namespace olp;
using namespace dataservice::read;
using namespace olp::tests::common;

constexpr auto HTTP_RESPONSE_QUADKEYS =
    R"jsonString({"subQuads": [{"subQuadKey": "4","version":282,"dataHandle":"7636348E50215979A39B5F3A429EDDB4.282","dataSize":277},{"subQuadKey":"5","version":282,"dataHandle":"8C9B3E08E294ADB2CD07EBC8412062FE.282","dataSize":271},{"subQuadKey": "6","version":282,"dataHandle":"9772F5E1822DFF25F48F150294B1ECF5.282","dataSize":289},{"subQuadKey":"7","version":282,"dataHandle":"BF84D8EC8124B96DBE5C4DB68B05918F.282","dataSize":283},{"subQuadKey":"1","version":48,"dataHandle":"BD53A6D60A34C20DC42ACAB2650FE361.48","dataSize":89}],"parentQuads":[{"partition":"23","version":282,"dataHandle":"F8F4C3CB09FBA61B927256CBCB8441D1.282","dataSize":52438},{"partition":"5","version":282,"dataHandle":"13E2C624E0136C3357D092EE7F231E87.282","dataSize":99151},{"partition":"95","version":253,"dataHandle":"B6F7614316BB8B81478ED7AE370B22A6.253","dataSize":6765}]})jsonString";
constexpr auto HTTP_RESPONSE_MAILFORMED =
    R"jsonString({"subQuads": 0}])jsonString";
constexpr auto HTTP_RESPONSE_WRONG_FORMAT =
    R"jsonString({"parentQuads": 0,"subQuads": 0})jsonString";

TEST(QuadTreeIndexTest, ParseBlob) {
  using testing::Return;
  {
    SCOPED_TRACE("Parse json and store to blob");
    auto tile_key = olp::geo::TileKey::FromHereTile("381");
    auto stream = std::stringstream(HTTP_RESPONSE_QUADKEYS);
    QuadTreeIndex index(tile_key, 1, stream);

    auto data = index.Find(tile_key);
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(data->data_handle, "BD53A6D60A34C20DC42ACAB2650FE361.48");
    EXPECT_EQ(data->tile_key, olp::geo::TileKey::FromHereTile("381"));
    EXPECT_EQ(data->version, 48);

    auto data2 = index.Find(olp::geo::TileKey::FromHereTile("95"));
    ASSERT_TRUE(data2.has_value());
    EXPECT_EQ(data2->data_handle, "B6F7614316BB8B81478ED7AE370B22A6.253");
    EXPECT_EQ(data2->tile_key, olp::geo::TileKey::FromHereTile("95"));
    EXPECT_EQ(data2->version, 253);

    auto data3 = index.Find(tile_key.AddedSubHereTile("2"));
    ASSERT_TRUE(data3.has_value());
    EXPECT_EQ(data3->data_handle, "9772F5E1822DFF25F48F150294B1ECF5.282");
    EXPECT_EQ(data3->tile_key, olp::geo::TileKey::FromHereTile("1526"));
    EXPECT_EQ(data3->version, 282);

    auto data4 = index.Find(tile_key.AddedSubHereTile("4"));
    ASSERT_TRUE(data4.has_value());
    EXPECT_EQ(data4->data_handle, "7636348E50215979A39B5F3A429EDDB4.282");
    EXPECT_EQ(data4->tile_key, olp::geo::TileKey::FromHereTile("1524"));
    EXPECT_EQ(data4->version, 282);
  }
}

TEST(QuadTreeIndexTest, ParseBlobMailformedJSon) {
  {
    SCOPED_TRACE("Mailformed JSon response");
    auto tile_key = olp::geo::TileKey::FromHereTile("381");
    auto stream = std::stringstream(HTTP_RESPONSE_MAILFORMED);
    QuadTreeIndex index(tile_key, 1, stream);
    auto data = index.Find(tile_key);
    EXPECT_FALSE(data.has_value());
  }

  {
    SCOPED_TRACE("Wrong Formatted JSon response");
    auto tile_key = olp::geo::TileKey::FromHereTile("381");
    auto stream = std::stringstream(HTTP_RESPONSE_WRONG_FORMAT);
    QuadTreeIndex index(tile_key, 1, stream);
    auto data = index.Find(tile_key);
    EXPECT_FALSE(data.has_value());
  }
}

}  // namespace
