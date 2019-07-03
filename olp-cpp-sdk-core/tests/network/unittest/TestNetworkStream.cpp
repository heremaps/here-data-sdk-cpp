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
#include <olp/core/network/NetworkStream_inl.h>
#include <map>

using namespace olp::network;

TEST(NetworkStringStream, construction_size) {
  NetworkStringOStream sos(32U, 64U);
  const NetworkStringBuf* sbuf = sos.rdbuf();
  GTEST_ASSERT_EQ(32U, sbuf->desired_size());
  GTEST_ASSERT_EQ(64U, sbuf->max_size());
}

TEST(NetworkStringStream, construction_buf) {
  NetworkStringIOStream sos(NetworkStringBuf(32U, 16U, 64U));
  const NetworkStringBuf* sbuf = sos.rdbuf();
  GTEST_ASSERT_EQ(32U, sbuf->desired_size());
  GTEST_ASSERT_EQ(16U, sbuf->size_increment());
  GTEST_ASSERT_EQ(64U, sbuf->max_size());
}

TEST(NetworkVectorStream, construction_size) {
  NetworkVectorOStream vos(32U, 64U);
  NetworkVectorBuf* vbuf = vos.rdbuf();
  GTEST_ASSERT_EQ(32U, vbuf->desired_size());
  GTEST_ASSERT_EQ(64U, vbuf->max_size());
}

TEST(NetworkVectorStream, construction_buf) {
  NetworkVectorIOStream vos(NetworkVectorBuf(32U, 3.0, 128U));
  NetworkVectorBuf* vbuf = vos.rdbuf();
  GTEST_ASSERT_EQ(32U, vbuf->desired_size());
  ASSERT_DOUBLE_EQ(3.0, vbuf->grow_factor());
  GTEST_ASSERT_EQ(128U, vbuf->max_size());
}

const char str1[] = "abcdefgh";

TEST(NetworkStringStream, read_write) {
  NetworkStringIOStream sios(8U, 12U);
  sios << str1;
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, sios.tellp());
  std::string s;
  sios >> s;
  ASSERT_STREQ(str1, s.c_str());
  GTEST_ASSERT_EQ(-1, sios.tellg());
  ASSERT_TRUE(sios.eof());
  sios.clear();
  sios.seekg(-1, std::ios_base::end);
  GTEST_ASSERT_EQ((int)sizeof(str1) - 2, sios.tellg());
  char c = (char)sios.get();
  GTEST_ASSERT_EQ(str1[sizeof(str1) - 2], c);
  sios.seekg(1);
  c = (char)sios.get();
  GTEST_ASSERT_EQ(str1[1], c);
  sios.seekp(1);
  ASSERT_TRUE(sios.fail());
  sios.clear();
  sios << str1;  // max size exceeded
  ASSERT_TRUE(sios.bad());
}

TEST(NetworkVectorStream, read_write) {
  NetworkVectorIOStream vios(8U, 12U);
  vios << str1;
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, vios.tellp());
  std::string s;
  vios >> s;
  ASSERT_STREQ(str1, s.c_str());
  GTEST_ASSERT_EQ(-1, vios.tellg());
  ASSERT_TRUE(vios.eof());
  vios.clear();
  vios.seekg(-1, std::ios_base::end);
  GTEST_ASSERT_EQ((int)sizeof(str1) - 2, vios.tellg());
  char c = (char)vios.get();
  GTEST_ASSERT_EQ(str1[sizeof(str1) - 2], c);
  vios.seekg(1);
  c = (char)vios.get();
  GTEST_ASSERT_EQ(str1[1], c);
  vios.seekp(1);
  ASSERT_TRUE(vios.fail());
  vios.clear();
  vios << str1;  // max size exceeded
  ASSERT_TRUE(vios.bad());
}

TEST(NetworkStringStream, move_construction) {
  NetworkStringOStream sos1(16U, 32U);
  sos1 << str1;
  NetworkStringOStream sos2(std::move(sos1));
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, sos2.tellp());
  ASSERT_STREQ(str1, sos2.cdata().c_str());
}

TEST(NetworkVectorStream, move_construction) {
  NetworkVectorOStream vos1(16U, 32U);
  vos1 << str1;
  NetworkVectorOStream vos2(std::move(vos1));
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, vos2.tellp());
  std::string tmp(vos2.cdata().begin(), vos2.cdata().end());
  ASSERT_STREQ(str1, tmp.c_str());
}

TEST(NetworkStringStream, move_assignment) {
  NetworkStringOStream sos1(16U, 32U);
  sos1 << str1;
  NetworkStringOStream sos2(0U, 0U);
  sos2 = std::move(sos1);
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, sos2.tellp());
  ASSERT_STREQ(str1, sos2.cdata().c_str());
}

TEST(NetworkVectorStream, move_assignment) {
  NetworkVectorOStream vos1(16U, 32U);
  vos1 << str1;
  NetworkVectorOStream vos2(0U, 0U);
  vos2 = std::move(vos1);
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, vos2.tellp());
  std::string tmp(vos2.cdata().begin(), vos2.cdata().end());
  ASSERT_STREQ(str1, tmp.c_str());
}

const char str2[] = "0123";

TEST(NetworkStringStream, swap) {
  NetworkStringOStream sos1(16U, 32U);
  sos1 << str1;
  NetworkStringOStream sos2(8U, 16U);
  sos2 << str2;
  swap(sos1, sos2);
  GTEST_ASSERT_EQ((int)sizeof(str2) - 1, sos1.tellp());
  ASSERT_STREQ(str2, sos1.cdata().c_str());
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, sos2.tellp());
  ASSERT_STREQ(str1, sos2.cdata().c_str());
}

TEST(NetworkVectorStream, swap) {
  NetworkVectorOStream vos1(16U, 32U);
  vos1 << str1;
  NetworkVectorOStream vos2(8U, 16U);
  vos2 << str2;
  swap(vos1, vos2);
  GTEST_ASSERT_EQ((int)sizeof(str2) - 1, vos1.tellp());
  std::string tmp1(vos1.cdata().begin(), vos1.cdata().end());
  ASSERT_STREQ(str2, tmp1.c_str());
  GTEST_ASSERT_EQ((int)sizeof(str1) - 1, vos2.tellp());
  std::string tmp2(vos2.cdata().begin(), vos2.cdata().end());
  ASSERT_STREQ(str1, tmp2.c_str());
}

using CB = Network::HeaderCallback;

TEST(NetworkStream, callback_good) {
  NetworkVectorOStream vos(0U, 4096U);
  CB cb = vos.header_func();
  cb("Server", "test");
  cb("CoNteNt-LeNgtH", "1024");
  GTEST_ASSERT_EQ(1024U, vos.rdbuf()->desired_size());
  GTEST_ASSERT_EQ(NetworkVectorOStream::SIZE_INCREMENT,
                  vos.rdbuf()->size_increment());
  vos.write("ABCD", 4);
  GTEST_ASSERT_EQ(1024U, vos.rdbuf()->capacity());
  ASSERT_TRUE(vos.good());
}

TEST(NetworkStream, callback_bad) {
  NetworkVectorOStream vos(0U, 4096U);
  CB cb = vos.header_func();
  cb("Server", "test");
  cb("Content-Length", "4097");
  GTEST_ASSERT_EQ(0U, vos.rdbuf()->desired_size());
  GTEST_ASSERT_EQ(0U, vos.rdbuf()->size_increment());
  vos.write("ABCD", 4);
  GTEST_ASSERT_EQ(0U, vos.rdbuf()->capacity());
  ASSERT_TRUE(vos.bad());
}

TEST(NetworkStream, callback_chained) {
  NetworkVectorOStream vos(0U, 4096U);
  std::map<std::string, std::string> map;
  CB cb = vos.header_func(
      [&map](const std::string& k, const std::string& v) -> void {
        map.emplace(k, v);
      });
  cb("Server", "test");
  cb("Content-Length", "1024");
  ASSERT_STREQ("test", map["Server"].c_str());
  ASSERT_STREQ("1024", map["Content-Length"].c_str());
}
