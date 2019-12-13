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

#include <chrono>
#include <fstream>
#include <string>
#include <thread>

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/utils/Dir.h>

TEST(DefaultCacheTest, BasicTest) {
  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
  ASSERT_TRUE(cache.Clear());
  std::string key1DataString{"this is key1's data"};
  cache.Put("key1", key1DataString, [=]() { return key1DataString; },
            (std::numeric_limits<time_t>::max)());
  auto key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1DataRead.empty());
  ASSERT_EQ(key1DataString, boost::any_cast<std::string>(key1DataRead));
  ASSERT_TRUE(cache.Clear());
}

TEST(DefaultCacheTest, BasicInMemTest) {
  olp::cache::DefaultCache cache;
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
  ASSERT_TRUE(cache.Clear());
  std::string key1DataString{"this is key1's data"};
  cache.Put("key1", key1DataString, [=]() { return key1DataString; },
            (std::numeric_limits<time_t>::max)());
  auto key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1DataRead.empty());
  ASSERT_EQ(key1DataString, boost::any_cast<std::string>(key1DataRead));
  ASSERT_TRUE(cache.Clear());
}

TEST(DefaultCacheTest, MemSizeTest) {
  olp::cache::CacheSettings settings;
  settings.max_memory_cache_size = 30;
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

  std::string key1{"key1"};
  {
    std::string key1DataString{"this is key1's data!"};  // char[20]
    cache.Put(key1, key1DataString, [=]() { return key1DataString; },
              (std::numeric_limits<time_t>::max)());
    auto key1DataRead =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1DataRead.empty());
    ASSERT_EQ(key1DataString, boost::any_cast<std::string>(key1DataRead));
  }

  std::string key2{"key2"};
  {
    std::string key2DataString{"this is key2's data!"};  // char[20]
    cache.Put(key2, key2DataString, [=]() { return key2DataString; },
              (std::numeric_limits<time_t>::max)());
    auto key2DataRead =
        cache.Get(key2, [](const std::string& data) { return data; });
    ASSERT_FALSE(key2DataRead.empty());
    ASSERT_EQ(key2DataString, boost::any_cast<std::string>(key2DataRead));

    auto key1DataRead =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_TRUE(key1DataRead.empty());
  }
}

TEST(DefaultCacheTest, RemoveWithPrefix) {
  olp::cache::DefaultCache cache;
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
  std::string dataString{"this is the data"};
  for (int i = 0; i < 11; i++) {
    cache.Put("key" + std::to_string(i), dataString,
              [=]() { return dataString; },
              (std::numeric_limits<time_t>::max)());
  }
  auto key10DataRead =
      cache.Get("key10", [](const std::string& data) { return data; });
  ASSERT_FALSE(key10DataRead.empty());
  cache.RemoveKeysWithPrefix("key1");  // removes "key1", "key10"
  key10DataRead =
      cache.Get("key10", [](const std::string& data) { return data; });
  ASSERT_TRUE(key10DataRead.empty());
  auto key4DataRead =
      cache.Get("key4", [](const std::string& data) { return data; });
  ASSERT_FALSE(key4DataRead.empty());
  cache.RemoveKeysWithPrefix("key4");  // removes "key4"
  key4DataRead =
      cache.Get("key4", [](const std::string& data) { return data; });
  ASSERT_TRUE(key4DataRead.empty());
  auto key2DataRead =
      cache.Get("key2", [](const std::string& data) { return data; });
  ASSERT_FALSE(key2DataRead.empty());
  cache.RemoveKeysWithPrefix("doesnotexist");  // removes nothing
  key2DataRead =
      cache.Get("key2", [](const std::string& data) { return data; });
  ASSERT_FALSE(key2DataRead.empty());
  cache.RemoveKeysWithPrefix("key");  // removes all
  key2DataRead =
      cache.Get("key2", [](const std::string& data) { return data; });
  ASSERT_TRUE(key2DataRead.empty());
}

TEST(DefaultCacheTest, BasicDiskTest) {
  olp::cache::CacheSettings settings;
  settings.max_memory_cache_size = 0;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
  ASSERT_TRUE(cache.Clear());
  std::string key1DataString{"this is key1's data"};
  cache.Put("key1", key1DataString, [=]() { return key1DataString; },
            (std::numeric_limits<time_t>::max)());
  auto key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1DataRead.empty());
  ASSERT_EQ(key1DataString, boost::any_cast<std::string>(key1DataRead));
  ASSERT_TRUE(cache.Clear());
}

TEST(DefaultCacheTest, ExpiredDiskTest) {
  olp::cache::CacheSettings settings;
  settings.max_memory_cache_size = 0;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
  ASSERT_TRUE(cache.Clear());
  std::string key1DataString{"this is key1's data"};
  // expired in the past, can't get it again
  cache.Put("key1", key1DataString, [=]() { return key1DataString; }, -1);
  auto key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_TRUE(key1DataRead.empty());

  // valid now, for 2 more seconds
  cache.Put("key1", key1DataString, [=]() { return key1DataString; }, 2);
  key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1DataRead.empty());
  std::this_thread::sleep_for(std::chrono::seconds(3));
  // should be invalid
  key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_TRUE(key1DataRead.empty());
  ASSERT_TRUE(cache.Clear());
}

TEST(DefaultCacheTest, ProtectedCacheTest) {
  const auto protected_path = olp::utils::Dir::TempDirectory() + "/protected";
  const std::string key1_data_string = "this is key1's data";
  const std::string key2_data_string = "this is key2's data";
  const std::string key1 = "key1";
  const std::string key2 = "key2";
  {
    SCOPED_TRACE("Setup cache");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = protected_path;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; },
              (std::numeric_limits<time_t>::max)());

    cache.Close();
  }
  {
    SCOPED_TRACE("Get from protected - success");
    olp::cache::CacheSettings settings;
    settings.disk_path_protected = protected_path;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
  }
  {
    SCOPED_TRACE("Get from protected - missing key");

    olp::cache::CacheSettings settings;
    settings.disk_path_protected = protected_path;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    auto key1_data_read =
        cache.Get(key2, [](const std::string& data) { return data; });
    ASSERT_TRUE(key1_data_read.empty());
  }
  {
    SCOPED_TRACE("Get from protected - fall-back to mutable");

    const std::string mutable_path =
        olp::utils::Dir::TempDirectory() + "/mutable";

    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = mutable_path;
    settings.disk_path_protected = protected_path;

    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    // Put to mutable
    cache.Put(key2, key2_data_string, [=]() { return key1_data_string; },
              (std::numeric_limits<time_t>::max)());

    auto key2_data_read =
        cache.Get(key2, [](const std::string& data) { return data; });
    ASSERT_FALSE(key2_data_read.empty());
    ASSERT_EQ(key2_data_string, boost::any_cast<std::string>(key2_data_read));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Remove from protected - blocked");
    olp::cache::CacheSettings settings;
    settings.disk_path_protected = protected_path;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    ASSERT_TRUE(cache.Remove(key1));

    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
  }
}

TEST(DefaultCacheTest, ExpiredMemTest) {
  olp::cache::DefaultCache cache;
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
  ASSERT_TRUE(cache.Clear());
  std::string key1DataString{"this is key1's data"};
  // expired in the past, can't get it again
  cache.Put("key1", key1DataString, [=]() { return key1DataString; }, -1);
  auto key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_TRUE(key1DataRead.empty());

  // valid now, for 2 more seconds
  cache.Put("key1", key1DataString, [=]() { return key1DataString; }, 2);
  key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1DataRead.empty());
  std::this_thread::sleep_for(std::chrono::seconds(3));
  // should be invalid
  key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_TRUE(key1DataRead.empty());
  ASSERT_TRUE(cache.Clear());
}

TEST(DefaultCacheTest, BadPathMutable) {
  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = std::string("/////this/is/a/bad/mutable/path");
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::OpenDiskPathFailure, cache.Open());

  // Verify that in-memory still works after bad path was set for memcache
  std::string key1DataString{"this is key1's data"};
  cache.Put("key1", key1DataString, [=]() { return key1DataString; },
            (std::numeric_limits<time_t>::max)());
  auto key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1DataRead.empty());
  ASSERT_EQ(key1DataString, boost::any_cast<std::string>(key1DataRead));
}

TEST(DefaultCacheTest, BadPathProtected) {
  olp::cache::CacheSettings settings;
  settings.disk_path_protected =
      std::string("/////this/is/a/bad/protected/path");
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::OpenDiskPathFailure, cache.Open());

  // Verify that in-memory still works after bad path was set for memcache
  std::string key1_data_string{"this is key1's data"};
  cache.Put("key1", key1_data_string, [=]() { return key1_data_string; },
            std::numeric_limits<time_t>::max());
  auto key1_data_read =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1_data_read.empty());
  ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
}

TEST(DefaultCacheTest, AlreadyInUsePath) {
  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

  olp::cache::DefaultCache cache2(settings);
  ASSERT_EQ(olp::cache::DefaultCache::OpenDiskPathFailure, cache2.Open());
}
