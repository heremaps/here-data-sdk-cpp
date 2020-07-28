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

#include <cache/ProtectedKeyList.h>

namespace {
namespace cache = olp::cache;
TEST(ProtectedKeyList, CanBeMoved) {
  SCOPED_TRACE("ProtectedKeyList can be moved");

  cache::ProtectedKeyList list_a;
  auto cb = [](const std::string&) {};
  list_a.Protect({"key"}, cb);
  EXPECT_EQ(list_a.Size(), 0);
  EXPECT_TRUE(list_a.IsDirty());
  auto raw_data = list_a.Serialize();
  EXPECT_TRUE(raw_data.get());
  EXPECT_EQ(raw_data->size(), 4);
  EXPECT_FALSE(list_a.IsDirty());
  EXPECT_EQ(list_a.Size(), raw_data->size());
  cache::ProtectedKeyList list_b(std::move(list_a));
  EXPECT_EQ(list_b.Size(), 4);
  cache::ProtectedKeyList list_c;
  EXPECT_EQ(list_c.Size(), 0);
  list_c = std::move(list_b);
  EXPECT_EQ(list_c.Size(), 4);
}

TEST(ProtectedKeyList, Protect) {
  {
    cache::ProtectedKeyList protected_keys;
    auto cb = [](const std::string&) {};
    {
      SCOPED_TRACE("Successful protect one key");
      EXPECT_FALSE(protected_keys.IsDirty());

      // protect key
      EXPECT_TRUE(protected_keys.Protect({"key:1"}, cb));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      EXPECT_EQ(raw_data->size(), 6);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_EQ(protected_keys.Size(), raw_data->size());
      protected_keys.Clear();
    }

    {
      SCOPED_TRACE("Protect prefix of previously protected key");
      EXPECT_TRUE(protected_keys.Protect({"key:"}, cb));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // previous key was removed, so size is smaller
      EXPECT_EQ(raw_data->size(), 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_TRUE(protected_keys.IsProtected("key:1"));
      protected_keys.Clear();
    }

    {
      SCOPED_TRACE("Protect new key with the its prefix already in cache");
      EXPECT_TRUE(protected_keys.Protect({"key:"}, cb));
      EXPECT_FALSE(protected_keys.Protect({"key:2"}, cb));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // size didn't change
      EXPECT_EQ(raw_data->size(), 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_TRUE(protected_keys.IsProtected("key:1"));
      protected_keys.Clear();
    }

    {
      SCOPED_TRACE("Protect new key with the other prefix");
      // old prefix
      EXPECT_TRUE(protected_keys.Protect({"key:"}, cb));
      // new prefix
      EXPECT_TRUE(protected_keys.Protect({"some_key:1"}, cb));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // size changed
      EXPECT_EQ(raw_data->size(), 5 + 11);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_TRUE(protected_keys.IsProtected("some_key:1"));
      protected_keys.Clear();
    }

    {
      SCOPED_TRACE("Protect multiple keys ");
      // old data
      EXPECT_TRUE(protected_keys.Protect({"key:", "some_key:1"}, cb));
      // new data
      EXPECT_TRUE(
          protected_keys.Protect({"some_key:2", "some_key:3", "some_key:4",
                                  "some_key:5", "some_key:6"},
                                 cb));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // size changed
      EXPECT_EQ(raw_data->size(), 5 + 11 + 11 * 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_TRUE(protected_keys.IsProtected("some_key:2"));
      EXPECT_FALSE(protected_keys.IsProtected("some_key:7"));
      protected_keys.Clear();
    }

    {
      SCOPED_TRACE("Protect multiple keys, its prefix already in cache ");
      // old data
      EXPECT_TRUE(protected_keys.Protect({"key:", "some_key:1"}, cb));
      EXPECT_TRUE(
          protected_keys.Protect({"some_key:2", "some_key:3", "some_key:4",
                                  "some_key:5", "some_key:6"},
                                 cb));
      // new data
      EXPECT_FALSE(protected_keys.Protect(
          {"key:2", "key:3", "key:4", "key:5", "key:6"}, cb));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // size didn't change
      EXPECT_EQ(raw_data->size(), 5 + 11 + 11 * 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      // this key is protected by prefix
      EXPECT_TRUE(protected_keys.IsProtected("key:7"));
      EXPECT_FALSE(protected_keys.IsProtected("some_key:7"));
      protected_keys.Clear();
    }
  }
}

TEST(ProtectedKeyList, Release) {
  {
    cache::ProtectedKeyList protected_keys;
    auto cb = [](const std::string&) {};
    {
      SCOPED_TRACE("Successful protect some keys and prefixes");
      EXPECT_FALSE(protected_keys.IsDirty());

      // protect keys
      EXPECT_TRUE(protected_keys.Protect(
          {"key:1", "some_key:1", "some_key:2", "some_key:3", "some_key:4",
           "some_key:5", "some_key:6", "key:"},
          cb));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // 6 keys and prefix
      EXPECT_EQ(raw_data->size(), 6 * 11 + 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_EQ(protected_keys.Size(), raw_data->size());
      // this key is protected by prefix
      EXPECT_TRUE(protected_keys.IsProtected("key:7"));
      EXPECT_TRUE(protected_keys.IsProtected("some_key:6"));
      EXPECT_FALSE(protected_keys.IsProtected("some_key:7"));
    }

    {
      SCOPED_TRACE("Release one key ");
      EXPECT_TRUE(protected_keys.Release({"some_key:6"}));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // 5 keys and prefix
      EXPECT_EQ(raw_data->size(), 5 * 11 + 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_EQ(protected_keys.Size(), raw_data->size());
      // key not longer protected
      EXPECT_FALSE(protected_keys.IsProtected("some_key:6"));
    }

    {
      SCOPED_TRACE("Release multiple keys ");
      EXPECT_TRUE(
          protected_keys.Release({"some_key:6", "some_key:5", "some_key:4"}));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // 5 keys and prefix
      EXPECT_EQ(raw_data->size(), 3 * 11 + 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_EQ(protected_keys.Size(), raw_data->size());
      // key not longer protected
      EXPECT_FALSE(protected_keys.IsProtected("some_key:5"));
    }

    {
      SCOPED_TRACE("Release keys by prefix");
      EXPECT_TRUE(protected_keys.Release({"some_key:"}));
      EXPECT_TRUE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // 5 keys and prefix
      EXPECT_EQ(raw_data->size(), 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_EQ(protected_keys.Size(), raw_data->size());
      // key not longer protected
      EXPECT_FALSE(protected_keys.IsProtected("some_key:1"));
      EXPECT_FALSE(protected_keys.IsProtected("some_key:2"));
    }

    {
      SCOPED_TRACE("Try to release key protected by prefix");
      EXPECT_FALSE(protected_keys.Release({"key:1"}));
      // nothing changed
      EXPECT_FALSE(protected_keys.IsDirty());
      auto raw_data = protected_keys.Serialize();
      EXPECT_TRUE(raw_data.get());
      // 5 keys and prefix
      EXPECT_EQ(raw_data->size(), 5);
      EXPECT_FALSE(protected_keys.IsDirty());
      EXPECT_EQ(protected_keys.Size(), raw_data->size());
      // key still protected
      EXPECT_TRUE(protected_keys.IsProtected("key:1"));
    }
  }
}

}  // namespace
