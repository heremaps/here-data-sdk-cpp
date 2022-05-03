/*
 * Copyright (C) 2020-2022 HERE Europe B.V.
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
namespace read = olp::dataservice::read;

constexpr auto HTTP_RESPONSE_QUADKEYS =
    R"jsonString({"subQuads": [{"subQuadKey": "4","version":282,"dataHandle":"7636348E50215979A39B5F3A429EDDB4.282","dataSize":277},{"subQuadKey":"5","version":282,"dataHandle":"8C9B3E08E294ADB2CD07EBC8412062FE.282","dataSize":271},{"subQuadKey": "6","version":282,"dataHandle":"9772F5E1822DFF25F48F150294B1ECF5.282","dataSize":289},{"subQuadKey":"7","version":282,"dataHandle":"BF84D8EC8124B96DBE5C4DB68B05918F.282","dataSize":283},{"subQuadKey":"1","version":48,"dataHandle":"BD53A6D60A34C20DC42ACAB2650FE361.48","dataSize":89}],"parentQuads":[{"partition":"23","version":282,"dataHandle":"F8F4C3CB09FBA61B927256CBCB8441D1.282","dataSize":52438},{"partition":"5","version":282,"dataHandle":"13E2C624E0136C3357D092EE7F231E87.282","dataSize":99151},{"partition":"95","version":253,"dataHandle":"B6F7614316BB8B81478ED7AE370B22A6.253","dataSize":6765}]})jsonString";
constexpr auto HTTP_RESPONSE_MAILFORMED =
    R"jsonString({"subQuads": 0}])jsonString";
constexpr auto HTTP_RESPONSE_WRONG_FORMAT =
    R"jsonString({"parentQuads": 0,"subQuads": 0})jsonString";

TEST(QuadTreeIndexTest, ParseBlob) {
  auto tile_key = olp::geo::TileKey::FromHereTile("381");
  auto stream = std::stringstream(HTTP_RESPONSE_QUADKEYS);
  read::QuadTreeIndex index(tile_key, 1, stream);

  {
    SCOPED_TRACE("Parse json and store to blob");

    auto data1 = index.Find(tile_key, false);
    ASSERT_FALSE(data1 == boost::none);
    EXPECT_EQ(data1->data_handle, "BD53A6D60A34C20DC42ACAB2650FE361.48");
    EXPECT_EQ(data1->tile_key, olp::geo::TileKey::FromHereTile("381"));
    EXPECT_EQ(data1->version, 48);

    auto data2 = index.Find(olp::geo::TileKey::FromHereTile("95"), false);
    ASSERT_FALSE(data2 == boost::none);
    EXPECT_EQ(data2->data_handle, "B6F7614316BB8B81478ED7AE370B22A6.253");
    EXPECT_EQ(data2->tile_key, olp::geo::TileKey::FromHereTile("95"));
    EXPECT_EQ(data2->version, 253);

    auto data3 = index.Find(tile_key.AddedSubHereTile("2"), false);
    ASSERT_FALSE(data3 == boost::none);
    EXPECT_EQ(data3->data_handle, "9772F5E1822DFF25F48F150294B1ECF5.282");
    EXPECT_EQ(data3->tile_key, olp::geo::TileKey::FromHereTile("1526"));
    EXPECT_EQ(data3->version, 282);

    auto data4 = index.Find(tile_key.AddedSubHereTile("4"), false);
    ASSERT_FALSE(data4 == boost::none);
    EXPECT_EQ(data4->data_handle, "7636348E50215979A39B5F3A429EDDB4.282");
    EXPECT_EQ(data4->tile_key, olp::geo::TileKey::FromHereTile("1524"));
    EXPECT_EQ(data4->version, 282);

    auto data5 = index.Find(olp::geo::TileKey::FromHereTile("1561298"), false);
    EXPECT_TRUE(data5 == boost::none);

    auto data6 = index.Find(olp::geo::TileKey::FromHereTile("3"), false);
    EXPECT_TRUE(data6 == boost::none);
  }

  {
    SCOPED_TRACE("Find Data Aggregated");

    // Find in parents
    auto data1 = index.Find(olp::geo::TileKey::FromHereTile("5842"), true);
    ASSERT_FALSE(data1 == boost::none);
    EXPECT_EQ(data1->data_handle, "13E2C624E0136C3357D092EE7F231E87.282");
    EXPECT_EQ(data1->tile_key, olp::geo::TileKey::FromHereTile("5"));
    EXPECT_EQ(data1->version, 282);

    // Find in sub quads
    auto data2 = index.Find(olp::geo::TileKey::FromHereTile("1561298"), true);
    ASSERT_FALSE(data2 == boost::none);
    EXPECT_EQ(data2->data_handle, "7636348E50215979A39B5F3A429EDDB4.282");
    EXPECT_EQ(data2->tile_key, olp::geo::TileKey::FromHereTile("1524"));
    EXPECT_EQ(data2->version, 282);

    // Use bottom tile so the algorithm will try to look in childs and parents
    // and will fail due to no nearest parent present in data
    auto data3 = index.Find(olp::geo::TileKey::FromHereTile("4818"), true);
    EXPECT_TRUE(data3 == boost::none);
  }

  {
    SCOPED_TRACE("Quad tree GetIndexData test");

    for (const auto& data : index.GetIndexData()) {
      if (data.tile_key == olp::geo::TileKey::FromHereTile("23")) {
        EXPECT_EQ(data.data_handle, "F8F4C3CB09FBA61B927256CBCB8441D1.282");
      }
      if (data.tile_key == olp::geo::TileKey::FromHereTile("1524")) {
        EXPECT_EQ(data.data_handle, "7636348E50215979A39B5F3A429EDDB4.282");
      }
    }
  }

  {
    SCOPED_TRACE("Malformed JSon response");

    auto tile_key = olp::geo::TileKey::FromHereTile("381");
    auto stream = std::stringstream(HTTP_RESPONSE_MAILFORMED);
    read::QuadTreeIndex index(tile_key, 1, stream);
    auto data = index.Find(tile_key, false);
    EXPECT_TRUE(data == boost::none);
  }

  {
    SCOPED_TRACE("Malformed QuadTreeIndex try to GetIndexData ");

    auto tile_key = olp::geo::TileKey::FromHereTile("381");
    auto stream = std::stringstream(HTTP_RESPONSE_MAILFORMED);
    read::QuadTreeIndex index(tile_key, 1, stream);
    EXPECT_EQ(index.GetIndexData().size(), 0);
  }

  {
    SCOPED_TRACE("Wrong Formatted JSon response");

    auto tile_key = olp::geo::TileKey::FromHereTile("381");
    auto stream = std::stringstream(HTTP_RESPONSE_WRONG_FORMAT);
    read::QuadTreeIndex index(tile_key, 1, stream);
    auto data = index.Find(tile_key, false);
    EXPECT_TRUE(data == boost::none);
  }
}

// The binary dump for HTTP_RESPONSE_QUADKEYS generated index, before the CRC
// field was introduced.
const char* quad_tree_index_dump =
    "\x7d\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x03\x05\x00\x00\x00"
    "\x01\x00\xcc\xcc\x68\x00\x00\x00\x04\x00\xcc\xcc\xa6\x00\x00\x00"
    "\x05\x00\xcc\xcc\xe5\x00\x00\x00\x06\x00\xcc\xcc\x24\x01\x00\x00"
    "\x07\x00\xcc\xcc\x63\x01\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00"
    "\xa2\x01\x00\x00\xcc\xcc\xcc\xcc\x17\x00\x00\x00\x00\x00\x00\x00"
    "\xe1\x01\x00\x00\xcc\xcc\xcc\xcc\x5f\x00\x00\x00\x00\x00\x00\x00"
    "\x20\x02\x00\x00\xcc\xcc\xcc\xcc\x30\x00\x00\x00\x00\x00\x00\x00"
    "\x59\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff"
    "\x42\x44\x35\x33\x41\x36\x44\x36\x30\x41\x33\x34\x43\x32\x30\x44"
    "\x43\x34\x32\x41\x43\x41\x42\x32\x36\x35\x30\x46\x45\x33\x36\x31"
    "\x2e\x34\x38\x00\x00\x00\x1a\x01\x00\x00\x00\x00\x00\x00\x15\x01"
    "\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x37\x36"
    "\x33\x36\x33\x34\x38\x45\x35\x30\x32\x31\x35\x39\x37\x39\x41\x33"
    "\x39\x42\x35\x46\x33\x41\x34\x32\x39\x45\x44\x44\x42\x34\x2e\x32"
    "\x38\x32\x00\x00\x00\x1a\x01\x00\x00\x00\x00\x00\x00\x0f\x01\x00"
    "\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x38\x43\x39"
    "\x42\x33\x45\x30\x38\x45\x32\x39\x34\x41\x44\x42\x32\x43\x44\x30"
    "\x37\x45\x42\x43\x38\x34\x31\x32\x30\x36\x32\x46\x45\x2e\x32\x38"
    "\x32\x00\x00\x00\x1a\x01\x00\x00\x00\x00\x00\x00\x21\x01\x00\x00"
    "\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x39\x37\x37\x32"
    "\x46\x35\x45\x31\x38\x32\x32\x44\x46\x46\x32\x35\x46\x34\x38\x46"
    "\x31\x35\x30\x32\x39\x34\x42\x31\x45\x43\x46\x35\x2e\x32\x38\x32"
    "\x00\x00\x00\x1a\x01\x00\x00\x00\x00\x00\x00\x1b\x01\x00\x00\x00"
    "\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x42\x46\x38\x34\x44"
    "\x38\x45\x43\x38\x31\x32\x34\x42\x39\x36\x44\x42\x45\x35\x43\x34"
    "\x44\x42\x36\x38\x42\x30\x35\x39\x31\x38\x46\x2e\x32\x38\x32\x00"
    "\x00\x00\x1a\x01\x00\x00\x00\x00\x00\x00\x4f\x83\x01\x00\x00\x00"
    "\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x31\x33\x45\x32\x43\x36"
    "\x32\x34\x45\x30\x31\x33\x36\x43\x33\x33\x35\x37\x44\x30\x39\x32"
    "\x45\x45\x37\x46\x32\x33\x31\x45\x38\x37\x2e\x32\x38\x32\x00\x00"
    "\x00\x1a\x01\x00\x00\x00\x00\x00\x00\xd6\xcc\x00\x00\x00\x00\x00"
    "\x00\xff\xff\xff\xff\xff\xff\xff\xff\x46\x38\x46\x34\x43\x33\x43"
    "\x42\x30\x39\x46\x42\x41\x36\x31\x42\x39\x32\x37\x32\x35\x36\x43"
    "\x42\x43\x42\x38\x34\x34\x31\x44\x31\x2e\x32\x38\x32\x00\x00\x00"
    "\xfd\x00\x00\x00\x00\x00\x00\x00\x6d\x1a\x00\x00\x00\x00\x00\x00"
    "\xff\xff\xff\xff\xff\xff\xff\xff\x42\x36\x46\x37\x36\x31\x34\x33"
    "\x31\x36\x42\x42\x38\x42\x38\x31\x34\x37\x38\x45\x44\x37\x41\x45"
    "\x33\x37\x30\x42\x32\x32\x41\x36\x2e\x32\x35\x33\x00\x00\x00";

const auto quad_tree_index_dump_len = 607;

TEST(QuadTreeIndexTest, BackwardsCompatibility) {
  auto tile_key = olp::geo::TileKey::FromHereTile("381");

  auto dump_data = std::make_shared<std::vector<unsigned char>>();
  dump_data->resize(quad_tree_index_dump_len);
  std::copy(quad_tree_index_dump,
            quad_tree_index_dump + quad_tree_index_dump_len, dump_data->data());

  read::QuadTreeIndex index(dump_data);

  {
    SCOPED_TRACE("Parse json and store to blob");

    auto data1 = index.Find(tile_key, false);
    ASSERT_FALSE(data1 == boost::none);
    EXPECT_EQ(data1->data_handle, "BD53A6D60A34C20DC42ACAB2650FE361.48");
    EXPECT_EQ(data1->tile_key, olp::geo::TileKey::FromHereTile("381"));
    EXPECT_EQ(data1->version, 48);

    auto data2 = index.Find(olp::geo::TileKey::FromHereTile("95"), false);
    ASSERT_FALSE(data2 == boost::none);
    EXPECT_EQ(data2->data_handle, "B6F7614316BB8B81478ED7AE370B22A6.253");
    EXPECT_EQ(data2->tile_key, olp::geo::TileKey::FromHereTile("95"));
    EXPECT_EQ(data2->version, 253);

    auto data3 = index.Find(tile_key.AddedSubHereTile("2"), false);
    ASSERT_FALSE(data3 == boost::none);
    EXPECT_EQ(data3->data_handle, "9772F5E1822DFF25F48F150294B1ECF5.282");
    EXPECT_EQ(data3->tile_key, olp::geo::TileKey::FromHereTile("1526"));
    EXPECT_EQ(data3->version, 282);

    auto data4 = index.Find(tile_key.AddedSubHereTile("4"), false);
    ASSERT_FALSE(data4 == boost::none);
    EXPECT_EQ(data4->data_handle, "7636348E50215979A39B5F3A429EDDB4.282");
    EXPECT_EQ(data4->tile_key, olp::geo::TileKey::FromHereTile("1524"));
    EXPECT_EQ(data4->version, 282);

    auto data5 = index.Find(olp::geo::TileKey::FromHereTile("1561298"), false);
    EXPECT_TRUE(data5 == boost::none);

    auto data6 = index.Find(olp::geo::TileKey::FromHereTile("3"), false);
    EXPECT_TRUE(data6 == boost::none);
  }

  {
    SCOPED_TRACE("Find Data Aggregated");

    // Find in parents
    auto data1 = index.Find(olp::geo::TileKey::FromHereTile("5842"), true);
    ASSERT_FALSE(data1 == boost::none);
    EXPECT_EQ(data1->data_handle, "13E2C624E0136C3357D092EE7F231E87.282");
    EXPECT_EQ(data1->tile_key, olp::geo::TileKey::FromHereTile("5"));
    EXPECT_EQ(data1->version, 282);

    // Find in sub quads
    auto data2 = index.Find(olp::geo::TileKey::FromHereTile("1561298"), true);
    ASSERT_FALSE(data2 == boost::none);
    EXPECT_EQ(data2->data_handle, "7636348E50215979A39B5F3A429EDDB4.282");
    EXPECT_EQ(data2->tile_key, olp::geo::TileKey::FromHereTile("1524"));
    EXPECT_EQ(data2->version, 282);

    // Use bottom tile so the algorithm will try to look in childs and parents
    // and will fail due to no nearest parent present in data
    auto data3 = index.Find(olp::geo::TileKey::FromHereTile("4818"), true);
    EXPECT_TRUE(data3 == boost::none);
  }

  {
    SCOPED_TRACE("Quad tree GetIndexData test");

    for (const auto& data : index.GetIndexData()) {
      if (data.tile_key == olp::geo::TileKey::FromHereTile("23")) {
        EXPECT_EQ(data.data_handle, "F8F4C3CB09FBA61B927256CBCB8441D1.282");
      }
      if (data.tile_key == olp::geo::TileKey::FromHereTile("1524")) {
        EXPECT_EQ(data.data_handle, "7636348E50215979A39B5F3A429EDDB4.282");
      }
    }
  }
}

}  // namespace
