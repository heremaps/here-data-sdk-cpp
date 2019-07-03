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
#include <olp/core/network/NetworkStreamBuf_inl.h>
#include <cstdint>

using uintptr_t = std::uintptr_t;

using namespace olp::network;

static const std::string empty;

TEST(NetworkStringBufTest, construction_increment) {
  NetworkStringBuf sbuf(1024U, 256U, 4096U);
  GTEST_ASSERT_EQ(1024U, sbuf.desired_size());
  GTEST_ASSERT_EQ(256U, sbuf.size_increment());
  ASSERT_DOUBLE_EQ(.0, sbuf.grow_factor());
  GTEST_ASSERT_EQ(4096U, sbuf.max_size());
  GTEST_ASSERT_EQ(0U, sbuf.size());
  GTEST_ASSERT_EQ(empty.capacity(), sbuf.capacity());
}

TEST(NetworkStringBufTest, construction_factor) {
  NetworkStringBuf sbuf(1024U, 2.0, 4096U);
  GTEST_ASSERT_EQ(1024U, sbuf.desired_size());
  GTEST_ASSERT_EQ(0U, sbuf.size_increment());
  ASSERT_DOUBLE_EQ(2.0, sbuf.grow_factor());
  GTEST_ASSERT_EQ(4096U, sbuf.max_size());
  GTEST_ASSERT_EQ(0U, sbuf.size());
  GTEST_ASSERT_EQ(empty.capacity(), sbuf.capacity());
}

TEST(NetworkVectorBufTest, construction_increment) {
  NetworkVectorBuf vbuf(1024U, 256U, 4096U);
  GTEST_ASSERT_EQ(1024U, vbuf.desired_size());
  GTEST_ASSERT_EQ(256U, vbuf.size_increment());
  ASSERT_DOUBLE_EQ(.0, vbuf.grow_factor());
  GTEST_ASSERT_EQ(4096U, vbuf.max_size());
  GTEST_ASSERT_EQ(0U, vbuf.size());
  GTEST_ASSERT_EQ(0U, vbuf.capacity());
}

TEST(NetworkVectorBufTest, construction_factor) {
  NetworkVectorBuf vbuf(1024U, 2.0, 4096U);
  GTEST_ASSERT_EQ(1024U, vbuf.desired_size());
  GTEST_ASSERT_EQ(0U, vbuf.size_increment());
  ASSERT_DOUBLE_EQ(2.0, vbuf.grow_factor());
  GTEST_ASSERT_EQ(4096U, vbuf.max_size());
  GTEST_ASSERT_EQ(0U, vbuf.size());
  GTEST_ASSERT_EQ(0U, vbuf.capacity());
}

TEST(NetworkStreamBufTest, construction_max_size) {
  NetworkStringBuf sbuf(1024U, 256U, NetworkStringBuf::MAX_SIZE + 1);
  GTEST_ASSERT_EQ(NetworkStringBuf::MAX_SIZE, sbuf.max_size());
  NetworkVectorBuf vbuf(1024U, 2.0, NetworkStringBuf::MAX_SIZE + 1);
  GTEST_ASSERT_EQ(NetworkStringBuf::MAX_SIZE, vbuf.max_size());
}

static const std::string s32("01234567890123456789012345678901");

TEST(NetworkStringBufTest, allocation_increment) {
  NetworkStringBuf sbuf(64U, 32U, 128U);
  int rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(32U, sbuf.size());
  // std::string might allocate more than requested
  GTEST_ASSERT_LE(64U, sbuf.capacity());
  GTEST_ASSERT_EQ(sbuf.cdata().capacity(), sbuf.capacity());
  rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(64U, sbuf.size());
  rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(96U, sbuf.size());
  GTEST_ASSERT_LE(96U, sbuf.capacity());
  GTEST_ASSERT_EQ(sbuf.cdata().capacity(), sbuf.capacity());
  rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(128U, sbuf.size());
  std::size_t capacity = sbuf.capacity();
  GTEST_ASSERT_LE(128U, capacity);
  for (int avl = static_cast<int>(capacity - 128U); 0 <= avl;) {
    rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
    if (-1 == rv) break;
    avl -= rv;
  }
  GTEST_ASSERT_EQ(-1, rv);
  // capacity should not be increased
  GTEST_ASSERT_LE(capacity, sbuf.capacity());
  GTEST_ASSERT_EQ(sbuf.cdata().capacity(), sbuf.capacity());
}

TEST(NetworkStringBufTest, allocation_factor) {
  NetworkStringBuf sbuf(64U, 2.0, 128U);
  int rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(32U, sbuf.size());
  GTEST_ASSERT_LE(64U, sbuf.capacity());
  GTEST_ASSERT_EQ(sbuf.cdata().capacity(), sbuf.capacity());
  rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(64U, sbuf.size());
  rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(96U, sbuf.size());
  GTEST_ASSERT_LE(96U, sbuf.capacity());
  GTEST_ASSERT_EQ(sbuf.cdata().capacity(), sbuf.capacity());
  rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(128U, sbuf.size());
  std::size_t capacity = sbuf.capacity();
  GTEST_ASSERT_LE(128U, capacity);
  for (int avl = static_cast<int>(capacity - 128U); 0 <= avl;) {
    rv = static_cast<int>(sbuf.sputn(s32.data(), s32.size()));
    if (-1 == rv) break;
    avl -= rv;
  }
  GTEST_ASSERT_EQ(-1, rv);
  // capacity should not be increased
  GTEST_ASSERT_LE(capacity, sbuf.capacity());
  GTEST_ASSERT_EQ(sbuf.cdata().capacity(), sbuf.capacity());
}

TEST(NetworkVectorBufTest, allocation_increment) {
  NetworkVectorBuf vbuf(64U, 32U, 128U);
  int rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(32U, vbuf.size());
  GTEST_ASSERT_EQ(64U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(64U, vbuf.size());
  GTEST_ASSERT_EQ(64U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(96U, vbuf.size());
  GTEST_ASSERT_EQ(96U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(128U, vbuf.size());
  GTEST_ASSERT_EQ(128U, vbuf.capacity());  // eof
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(-1, rv);
  GTEST_ASSERT_EQ(128U, vbuf.size());
  GTEST_ASSERT_EQ(128U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
}

TEST(NetworkVectorBufTest, allocation_factor) {
  NetworkVectorBuf vbuf(64U, 2.0, 128U);
  int rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(32U, vbuf.size());
  GTEST_ASSERT_EQ(64U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(64U, vbuf.size());
  GTEST_ASSERT_EQ(64U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(96U, vbuf.size());
  GTEST_ASSERT_EQ(128U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(32, rv);
  GTEST_ASSERT_EQ(128U, vbuf.size());
  GTEST_ASSERT_EQ(128U, vbuf.capacity());  // eof
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
  rv = static_cast<int>(vbuf.sputn(s32.data(), s32.size()));
  GTEST_ASSERT_EQ(-1, rv);
  GTEST_ASSERT_EQ(128U, vbuf.size());
  GTEST_ASSERT_EQ(128U, vbuf.capacity());
  GTEST_ASSERT_EQ(vbuf.cdata().capacity(), vbuf.capacity());
}

TEST(NetworkStreamBufTest, size_settings) {
  NetworkStringBuf sbuf(64U, 32U, 128U);
  ASSERT_TRUE(sbuf.set_max_size(256U));
  GTEST_ASSERT_EQ(256U, sbuf.max_size());
  ASSERT_FALSE(sbuf.set_max_size(NetworkStringBuf::MAX_SIZE + 1U));
  GTEST_ASSERT_EQ(256U, sbuf.max_size());
  ASSERT_TRUE(sbuf.set_desired_size(96U));
  GTEST_ASSERT_EQ(96U, sbuf.desired_size());
  ASSERT_FALSE(sbuf.set_desired_size(1024U));
  GTEST_ASSERT_EQ(96U, sbuf.desired_size());
  ASSERT_TRUE(sbuf.set_size_increment(64U));
  GTEST_ASSERT_EQ(64U, sbuf.size_increment());
  ASSERT_DOUBLE_EQ(.0, sbuf.grow_factor());
  ASSERT_FALSE(sbuf.set_size_increment(NetworkStringBuf::MAX_INCR + 1U));
  GTEST_ASSERT_EQ(64U, sbuf.size_increment());
  ASSERT_TRUE(sbuf.set_grow_factor(4.0));
  ASSERT_DOUBLE_EQ(4.0, sbuf.grow_factor());
  GTEST_ASSERT_EQ(0U, sbuf.size_increment());
  ASSERT_FALSE(sbuf.set_grow_factor(1.0));
  ASSERT_DOUBLE_EQ(4.0, sbuf.grow_factor());
}

// seeking not allowed
TEST(NetworkStreamBufTest, put_area) {
  NetworkVectorBuf vbuf(32U, 2.0, 64U);
  vbuf.sputn(s32.data(), s32.size());
  GTEST_ASSERT_EQ(32,
                  vbuf.pubseekoff(32, std::ios_base::beg, std::ios_base::out));
  GTEST_ASSERT_EQ(32,
                  vbuf.pubseekoff(0, std::ios_base::cur, std::ios_base::out));
  GTEST_ASSERT_EQ(32,
                  vbuf.pubseekoff(0, std::ios_base::end, std::ios_base::out));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(31, std::ios_base::beg, std::ios_base::out));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(33, std::ios_base::beg, std::ios_base::out));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(-1, std::ios_base::cur, std::ios_base::out));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(1, std::ios_base::cur, std::ios_base::out));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(-1, std::ios_base::end, std::ios_base::out));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(1, std::ios_base::end, std::ios_base::out));
  vbuf.sputc('2');
  GTEST_ASSERT_EQ(33,
                  vbuf.pubseekoff(0, std::ios_base::cur, std::ios_base::out));
}

// seeking allowed
TEST(NetworkStreamBufTest, get_area) {
  NetworkVectorBuf vbuf(32U, 2.0, 64U);
  vbuf.sputn(s32.data(), s32.size());
  int c = vbuf.sbumpc();
  GTEST_ASSERT_EQ((int)'0', c);
  GTEST_ASSERT_EQ(1, vbuf.pubseekoff(0, std::ios_base::cur, std::ios_base::in));
  GTEST_ASSERT_EQ(10,
                  vbuf.pubseekoff(10, std::ios_base::beg, std::ios_base::in));
  char gout[4];
  int rv = static_cast<int>(vbuf.sgetn(gout, 3));
  GTEST_ASSERT_EQ(3, rv);
  gout[3] = 0;
  ASSERT_STREQ("012", gout);
  GTEST_ASSERT_EQ(31,
                  vbuf.pubseekoff(31, std::ios_base::beg, std::ios_base::in));
  c = vbuf.sbumpc();
  GTEST_ASSERT_EQ((int)'1', c);
  GTEST_ASSERT_EQ(
      32, vbuf.pubseekoff(0, std::ios_base::cur, std::ios_base::in));  // at eof
  c = vbuf.sbumpc();
  GTEST_ASSERT_EQ(-1, c);  // eof read
  GTEST_ASSERT_EQ(32,
                  vbuf.pubseekoff(0, std::ios_base::cur, std::ios_base::in));
  vbuf.sputn(s32.data() + 2, 8);
  c = vbuf.sbumpc();
  GTEST_ASSERT_EQ((int)'2', c);
  GTEST_ASSERT_EQ(33,
                  vbuf.pubseekoff(0, std::ios_base::cur, std::ios_base::in));
  GTEST_ASSERT_EQ(39,
                  vbuf.pubseekoff(6, std::ios_base::cur, std::ios_base::in));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(7, std::ios_base::cur, std::ios_base::in));
  GTEST_ASSERT_EQ(0,
                  vbuf.pubseekoff(-39, std::ios_base::cur, std::ios_base::in));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(-1, std::ios_base::cur, std::ios_base::in));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(41, std::ios_base::beg, std::ios_base::in));
  GTEST_ASSERT_EQ(40,
                  vbuf.pubseekoff(40, std::ios_base::beg, std::ios_base::in));
  GTEST_ASSERT_EQ(-1,
                  vbuf.pubseekoff(1, std::ios_base::end, std::ios_base::in));
  GTEST_ASSERT_EQ(38,
                  vbuf.pubseekoff(-2, std::ios_base::end, std::ios_base::in));
  GTEST_ASSERT_EQ(-1, vbuf.pubseekpos(-1, std::ios_base::in));
  GTEST_ASSERT_EQ(-1, vbuf.pubseekpos(41, std::ios_base::in));
  GTEST_ASSERT_EQ(20, vbuf.pubseekpos(20, std::ios_base::in));
  c = vbuf.sbumpc();
  GTEST_ASSERT_EQ((int)'0', c);
}

TEST(NetworkStringBufTest, move_construction) {
  NetworkStringBuf sbuf1(32U, 16U, 64U);
  sbuf1.sputn(s32.data(), s32.size());
  uintptr_t sp1 = (uintptr_t)sbuf1.data().data();
  NetworkStringBuf sbuf2(std::move(sbuf1));
  GTEST_ASSERT_EQ(32U, sbuf2.size());
  GTEST_ASSERT_EQ(32U, sbuf2.desired_size());
  GTEST_ASSERT_EQ(16U, sbuf2.size_increment());
  GTEST_ASSERT_EQ(64U, sbuf2.max_size());
  uintptr_t sp2 = (uintptr_t)sbuf2.data().data();
  GTEST_ASSERT_EQ(sp1, sp2);
}

TEST(NetworkVectorBufTest, move_construction) {
  NetworkVectorBuf vbuf1(32U, 16U, 64U);
  vbuf1.sputn(s32.data(), s32.size());
  uintptr_t vp1 = (uintptr_t)vbuf1.data().data();
  NetworkVectorBuf vbuf2(std::move(vbuf1));
  GTEST_ASSERT_EQ(32U, vbuf2.size());
  GTEST_ASSERT_EQ(32U, vbuf2.desired_size());
  GTEST_ASSERT_EQ(16U, vbuf2.size_increment());
  GTEST_ASSERT_EQ(64U, vbuf2.max_size());
  uintptr_t vp2 = (uintptr_t)vbuf2.data().data();
  GTEST_ASSERT_EQ(vp1, vp2);
}

TEST(NetworkStringBufTest, move_assignment) {
  NetworkStringBuf sbuf1(32U, 16U, 64U);
  sbuf1.sputn(s32.data(), s32.size());
  uintptr_t sp1 = (uintptr_t)sbuf1.data().data();
  NetworkStringBuf sbuf2(0U, 0U, 0U);
  sbuf2 = std::move(sbuf1);
  GTEST_ASSERT_EQ(32U, sbuf2.size());
  GTEST_ASSERT_EQ(32U, sbuf2.desired_size());
  GTEST_ASSERT_EQ(16U, sbuf2.size_increment());
  GTEST_ASSERT_EQ(64U, sbuf2.max_size());
  uintptr_t sp2 = (uintptr_t)sbuf2.data().data();
  GTEST_ASSERT_EQ(sp1, sp2);
}

TEST(NetworkVectorBufTest, move_assignment) {
  NetworkVectorBuf vbuf1(32U, 16U, 64U);
  vbuf1.sputn(s32.data(), s32.size());
  uintptr_t vp1 = (uintptr_t)vbuf1.data().data();
  NetworkVectorBuf vbuf2(0U, 0U, 0U);
  vbuf2 = std::move(vbuf1);
  GTEST_ASSERT_EQ(32U, vbuf2.size());
  GTEST_ASSERT_EQ(32U, vbuf2.desired_size());
  GTEST_ASSERT_EQ(16U, vbuf2.size_increment());
  GTEST_ASSERT_EQ(64U, vbuf2.max_size());
  uintptr_t vp2 = (uintptr_t)vbuf2.data().data();
  GTEST_ASSERT_EQ(vp1, vp2);
}

TEST(NetworkStringBufTest, swap) {
  NetworkStringBuf sbuf1(32U, 16U, 64U);
  sbuf1.sputn(s32.data(), s32.size());
  uintptr_t sp1 = (uintptr_t)sbuf1.data().data();

  NetworkStringBuf sbuf2(24U, 8U, 32U);
  sbuf2.sputn(s32.data(), 24);
  uintptr_t sp2 = (uintptr_t)sbuf2.data().data();

  swap(sbuf1, sbuf2);

  GTEST_ASSERT_EQ(32U, sbuf2.size());
  GTEST_ASSERT_EQ(32U, sbuf2.desired_size());
  GTEST_ASSERT_EQ(16U, sbuf2.size_increment());
  GTEST_ASSERT_EQ(64U, sbuf2.max_size());
  uintptr_t sp2s = (uintptr_t)sbuf2.data().data();
  GTEST_ASSERT_EQ(sp1, sp2s);

  GTEST_ASSERT_EQ(24U, sbuf1.size());
  GTEST_ASSERT_EQ(24U, sbuf1.desired_size());
  GTEST_ASSERT_EQ(8U, sbuf1.size_increment());
  GTEST_ASSERT_EQ(32U, sbuf1.max_size());
  uintptr_t sp1s = (uintptr_t)sbuf1.data().data();
  GTEST_ASSERT_EQ(sp2, sp1s);
}

TEST(NetworkVectorBufTest, swap) {
  NetworkVectorBuf vbuf1(32U, 16U, 64U);
  vbuf1.sputn(s32.data(), s32.size());
  uintptr_t vp1 = (uintptr_t)vbuf1.data().data();

  NetworkVectorBuf vbuf2(24U, 8U, 32U);
  vbuf2.sputn(s32.data(), 24);
  uintptr_t vp2 = (uintptr_t)vbuf2.data().data();

  swap(vbuf1, vbuf2);

  GTEST_ASSERT_EQ(32U, vbuf2.size());
  GTEST_ASSERT_EQ(32U, vbuf2.desired_size());
  GTEST_ASSERT_EQ(16U, vbuf2.size_increment());
  GTEST_ASSERT_EQ(64U, vbuf2.max_size());
  uintptr_t vp2s = (uintptr_t)vbuf2.data().data();
  GTEST_ASSERT_EQ(vp1, vp2s);

  GTEST_ASSERT_EQ(24U, vbuf1.size());
  GTEST_ASSERT_EQ(24U, vbuf1.desired_size());
  GTEST_ASSERT_EQ(8U, vbuf1.size_increment());
  GTEST_ASSERT_EQ(32U, vbuf1.max_size());
  uintptr_t vp1s = (uintptr_t)vbuf1.data().data();
  GTEST_ASSERT_EQ(vp2, vp1s);
}

TEST(NetworkStringBufTest, move_storage) {
  NetworkStringBuf sbuf(32U, 16U, 64U);
  sbuf.sputn(s32.data(), s32.size());
  uintptr_t sp1 = (uintptr_t)sbuf.data().data();
  std::string s(std::move(sbuf.data()));
  GTEST_ASSERT_EQ(32U, s.size());
  uintptr_t sp2 = (uintptr_t)s.data();
  GTEST_ASSERT_EQ(sp1, sp2);
}

TEST(NetworkVectorBufTest, move_storage) {
  NetworkVectorBuf vbuf(32U, 16U, 64U);
  vbuf.sputn(s32.data(), s32.size());
  uintptr_t vp1 = (uintptr_t)vbuf.data().data();
  std::vector<char> v(std::move(vbuf.data()));
  GTEST_ASSERT_EQ(32U, v.size());
  uintptr_t vp2 = (uintptr_t)v.data();
  GTEST_ASSERT_EQ(vp1, vp2);
}
