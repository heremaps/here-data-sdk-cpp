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
#include "CancellationTokenList.h"

namespace {

using namespace olp::dataservice::write;

TEST(CancellationTokenListTest, Basic) {
  CancellationTokenList list;
  list.CancelAll();

  bool b0 = false;
  bool b1 = false;
  bool b2 = false;
  bool b3 = false;
  bool b4 = false;

  list.AddTask(3, olp::client::CancellationToken([&b3]() { b3 = true; }));
  list.AddTask(0, olp::client::CancellationToken([&b0]() { b0 = true; }));
  list.AddTask(1, olp::client::CancellationToken([&b1]() { b1 = true; }));
  list.AddTask(4, olp::client::CancellationToken([&b4]() { b4 = true; }));
  list.AddTask(2, olp::client::CancellationToken([&b2]() { b2 = true; }));

  list.RemoveTask(0);
  list.RemoveTask(2);

  list.CancelAll();

  ASSERT_FALSE(b0);
  ASSERT_TRUE(b1);
  ASSERT_FALSE(b2);
  ASSERT_TRUE(b3);
  ASSERT_TRUE(b4);
}

TEST(CancellationTokenListTest, Basic2) {
  CancellationTokenList list;

  bool b0 = false;
  bool b1 = false;
  bool b2 = false;
  bool b3 = false;
  bool b4 = false;

  list.AddTask(3, olp::client::CancellationToken([&b3]() { b3 = true; }));
  list.AddTask(0, olp::client::CancellationToken([&b0]() { b0 = true; }));
  list.AddTask(1, olp::client::CancellationToken([&b1]() { b1 = true; }));
  list.AddTask(4, olp::client::CancellationToken([&b4]() { b4 = true; }));
  list.AddTask(2, olp::client::CancellationToken([&b2]() { b2 = true; }));

  list.CancelAll();

  ASSERT_TRUE(b0);
  ASSERT_TRUE(b1);
  ASSERT_TRUE(b2);
  ASSERT_TRUE(b3);
  ASSERT_TRUE(b4);
}

TEST(CancellationTokenListTest, Basic3) {
  CancellationTokenList list;

  bool b0 = false;
  bool b1 = false;
  bool b2 = false;
  bool b3 = false;
  bool b4 = false;

  list.AddTask(3, olp::client::CancellationToken([&b3]() { b3 = true; }));
  list.AddTask(0, olp::client::CancellationToken([&b0]() { b0 = true; }));
  list.AddTask(1, olp::client::CancellationToken([&b1]() { b1 = true; }));
  list.AddTask(4, olp::client::CancellationToken([&b4]() { b4 = true; }));
  list.AddTask(2, olp::client::CancellationToken([&b2]() { b2 = true; }));

  list.RemoveTask(4);
  list.RemoveTask(0);
  list.RemoveTask(1);
  list.RemoveTask(2);

  list.CancelAll();

  ASSERT_FALSE(b0);
  ASSERT_FALSE(b1);
  ASSERT_FALSE(b2);
  ASSERT_TRUE(b3);
  ASSERT_FALSE(b4);
}

}  // namespace
