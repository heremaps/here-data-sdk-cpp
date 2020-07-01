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

void BasicCacheTestWithSettings(const olp::cache::CacheSettings& settings) {
  {
    SCOPED_TRACE("Put/Get decode");

    std::string data_string{"this is key's data"};
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());

    bool put_result =
        cache.Put("key", data_string, [=]() { return data_string; },
                  (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(put_result);

    auto data_read =
        cache.Get("key", [](const std::string& data) { return data; });

    EXPECT_FALSE(data_read.empty());
    EXPECT_EQ(data_string, boost::any_cast<std::string>(data_read));
  }

  {
    SCOPED_TRACE("Put/Get binary");

    std::vector<unsigned char> binary_data = {1, 2, 3};
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());

    bool put_result = cache.Put(
        "key", std::make_shared<std::vector<unsigned char>>(binary_data),
        (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(put_result);

    auto data_read = cache.Get("key");

    ASSERT_TRUE(data_read);
    EXPECT_EQ(*data_read, binary_data);
  }

  {
    SCOPED_TRACE("Remove from cache");

    std::vector<unsigned char> binary_data = {1, 2, 3};
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());

    bool put_result = cache.Put(
        "key", std::make_shared<std::vector<unsigned char>>(binary_data),
        (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(put_result);

    auto data_read = cache.Get("key");

    ASSERT_TRUE(data_read);
    EXPECT_EQ(*data_read, binary_data);

    // check removing missing key is not an error.
    EXPECT_TRUE(cache.Remove("invalid_key"));
    EXPECT_TRUE(cache.Remove("key"));

    data_read = cache.Get("key");

    EXPECT_FALSE(data_read);
  }

  {
    SCOPED_TRACE("RemoveWithPrefix");

    std::vector<unsigned char> binary_data = {1, 2, 3};
    std::string data_string{"this is key1's data"};
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());

    cache.Put("key1", data_string, [=]() { return data_string; },
              (std::numeric_limits<time_t>::max)());
    cache.Put("somekey1",
              std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());
    cache.Put("somekey2",
              std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());

    ASSERT_FALSE(cache.Get("key1", [](const std::string& data) { return data; })
                     .empty());
    ASSERT_TRUE(cache.Get("somekey1"));
    ASSERT_TRUE(cache.Get("somekey2"));

    auto result = cache.RemoveKeysWithPrefix("invalid_prefix");

    ASSERT_TRUE(result);
    ASSERT_FALSE(cache.Get("key1", [](const std::string& data) { return data; })
                     .empty());
    ASSERT_TRUE(cache.Get("somekey1"));
    ASSERT_TRUE(cache.Get("somekey2"));

    result = cache.RemoveKeysWithPrefix("key");

    ASSERT_TRUE(result);
    ASSERT_TRUE(cache.Get("key1", [](const std::string& data) { return data; })
                    .empty());
    ASSERT_TRUE(cache.Get("somekey1"));
    ASSERT_TRUE(cache.Get("somekey2"));

    result = cache.RemoveKeysWithPrefix("somekey");

    ASSERT_TRUE(result);
    ASSERT_FALSE(cache.Get("somekey1"));
    ASSERT_FALSE(cache.Get("somekey2"));
  }

  {
    SCOPED_TRACE("Clear");

    std::vector<unsigned char> binary_data = {1, 2, 3};
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());

    bool put_result = cache.Put(
        "key", std::make_shared<std::vector<unsigned char>>(binary_data),
        (std::numeric_limits<time_t>::max)());

    ASSERT_TRUE(put_result);

    auto result = cache.Clear();
    auto data_read = cache.Get("key");

    ASSERT_TRUE(result);
    ASSERT_FALSE(data_read);
  }

  {
    SCOPED_TRACE("Load disk cache");

    std::vector<unsigned char> binary_data = {1, 2, 3};
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());

    bool put_result = cache.Put(
        "key", std::make_shared<std::vector<unsigned char>>(binary_data),
        (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(put_result);

    cache.Close();
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    auto data_read = cache.Get("key");

    ASSERT_TRUE(data_read);
    ASSERT_EQ(*data_read, binary_data);
  }
}

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

  cache.Close();
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

  auto key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_TRUE(key1DataRead.empty());

  // valid now, for 2 more seconds
  cache.Put("key1", key1DataString, [=]() { return key1DataString; }, 2);

  cache.Close();
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

  key1DataRead =
      cache.Get("key1", [](const std::string& data) { return data; });
  ASSERT_FALSE(key1DataRead.empty());

  cache.Close();
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

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

    auto key2_data_read =
        cache.Get(key2, [](const std::string& data) { return data; });
    ASSERT_TRUE(key2_data_read.empty());
  }
  {
    SCOPED_TRACE("Get from protected - fall-back to mutable");

    const std::string mutable_path =
        olp::utils::Dir::TempDirectory() + "/mutable";

    olp::cache::CacheSettings settings;
    settings.max_memory_cache_size = 0;
    settings.disk_path_mutable = mutable_path;
    settings.disk_path_protected = protected_path;

    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    // Put to mutable
    cache.Put(key2, key2_data_string, [=]() { return key2_data_string; },
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
  {
    SCOPED_TRACE("Put to protected - blocked");
    olp::cache::CacheSettings settings;
    settings.disk_path_protected = protected_path;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    // Put and clear
    cache.Put(key2, key2_data_string, [=]() { return key2_data_string; },
              (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(cache.Clear());

    // key2 is missing for protected cache
    auto key2_data_read =
        cache.Get(key2, [](const std::string& data) { return data; });
    ASSERT_TRUE(key2_data_read.empty());

    // Check if key1 is still in protected
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

TEST(DefaultCacheTest, AlreadyInUsePath) {
  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  olp::cache::DefaultCache cache(settings);
  ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

  olp::cache::DefaultCache cache2(settings);
  ASSERT_EQ(olp::cache::DefaultCache::OpenDiskPathFailure, cache2.Open());
}

TEST(DefaultCacheTest, ValueGreaterThanMemCacheLimit) {
  const std::string content_key = "test_key";
  const std::string content =
      "a very long string that does not fit into the in memory cache";

  olp::cache::CacheSettings settings;
  settings.max_memory_cache_size = 10;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/mutable";

  olp::cache::DefaultCache cache(settings);
  EXPECT_EQ(cache.Open(), olp::cache::DefaultCache::StorageOpenResult::Success);

  auto input_buffer = std::make_shared<std::vector<unsigned char>>(
      std::begin(content), std::end(content));
  EXPECT_TRUE(cache.Put(content_key, input_buffer, 15));

  auto output_buffer = cache.Get(content_key);
  ASSERT_TRUE(output_buffer != nullptr);

  EXPECT_TRUE(std::equal(output_buffer->begin(), output_buffer->end(),
                         content.begin()));

  cache.Close();
}

TEST(DefaultCacheTest, EvictionPolicy) {
  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  settings.max_memory_cache_size = 0;

  settings.eviction_policy = olp::cache::EvictionPolicy::kNone;
  BasicCacheTestWithSettings(settings);

  settings.eviction_policy = olp::cache::EvictionPolicy::kLeastRecentlyUsed;
  BasicCacheTestWithSettings(settings);
}

TEST(DefaultCacheTest, CheckIfKeyExist) {
  const std::string key1_data_string = "this is key1's data";
  const std::string key2_data_string = "this is key2's data";
  const std::string key1 = "key1";
  const std::string key2 = "key2";
  {
    SCOPED_TRACE("Check key exist cache with lru");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    settings.max_memory_cache_size = 0;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; },
              (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_FALSE(cache.Contains(key2));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check key exist cache mutable");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    settings.eviction_policy = olp::cache::EvictionPolicy::kNone;
    settings.max_memory_cache_size = 0;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; },
              (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_FALSE(cache.Contains(key2));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check key exist cache protected");
    // Setup cache
    const auto protected_path = olp::utils::Dir::TempDirectory() + "/protected";
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = protected_path;
    olp::cache::DefaultCache cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());

    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; },
              (std::numeric_limits<time_t>::max)());

    cache.Close();

    settings = olp::cache::CacheSettings();
    settings.disk_path_protected = protected_path;
    settings.eviction_policy = olp::cache::EvictionPolicy::kNone;
    olp::cache::DefaultCache cache_protected(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache_protected.Open());
    ASSERT_TRUE(cache_protected.Contains(key1));
    ASSERT_FALSE(cache_protected.Contains(key2));
    ASSERT_TRUE(cache_protected.Clear());
  }
  {
    SCOPED_TRACE("Check key exist in memory cache");
    olp::cache::DefaultCache cache;
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; },
              (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_FALSE(cache.Contains(key2));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check key exist in memory cache after data expired");
    olp::cache::DefaultCache cache;
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_FALSE(cache.Contains(key1));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check key exist closed cache");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    olp::cache::DefaultCache cache(settings);
    ASSERT_FALSE(cache.Contains(key1));
  }
}
