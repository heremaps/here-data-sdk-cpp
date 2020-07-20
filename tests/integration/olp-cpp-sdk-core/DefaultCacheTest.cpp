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
#include <thread>

#include <olp/core/cache/DefaultCache.h>

namespace {
namespace cache = olp::cache;

TEST(DefaultCacheTest, DataExpiration) {
  const std::string content_key = "test_key";
  cache::CacheSettings settings;
  settings.max_memory_cache_size = 5;  // bytes
  settings.disk_path_mutable = "./cache";
  const auto expire_time = 1;

  {
    SCOPED_TRACE("Create a disk cache, write data.");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);

    const std::string content = "12345";
    auto buffer = std::make_shared<std::vector<unsigned char>>(
        std::begin(content), std::end(content));
    EXPECT_TRUE(cache.Put(content_key, buffer, expire_time));
    // check if data is in cache
    auto buffer_out_1 = cache.Get(content_key);
    ASSERT_NE(buffer_out_1, nullptr);
    cache.Close();
  }

  std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));

  {
    SCOPED_TRACE("Check that data is expired after timeout.");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);

    auto buffer = cache.Get(content_key);
    ASSERT_EQ(buffer, nullptr);

    cache.Close();
  }
}

TEST(DefaultCacheTest, ProtectExpiration) {
  const std::string prefix = "test::";
  const std::string content_key = prefix + "test_key";
  cache::CacheSettings settings;
  settings.max_memory_cache_size = 5;  // bytes
  settings.disk_path_mutable = "./cache";
  const auto expire_time = 1;
  const std::string content = "12345";
  auto buffer = std::make_shared<std::vector<unsigned char>>(
      std::begin(content), std::end(content));

  {
    SCOPED_TRACE("Protected data do not expire after timeout.");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    EXPECT_TRUE(cache.Put(content_key, buffer, expire_time));
    // protect key wait timeout and check if it exist
    EXPECT_TRUE(cache.Protect({content_key}));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    ASSERT_NE(cache.Get(content_key), nullptr);
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protected data do not expire after release.");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    EXPECT_TRUE(cache.Put(content_key, buffer, expire_time));
    // protect key wait timeout and check if it exist
    EXPECT_TRUE(cache.Protect({content_key}));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    EXPECT_TRUE(cache.Contains(content_key));
    ASSERT_NE(cache.Get(content_key), nullptr);
    // release key and check if it expires
    EXPECT_TRUE(cache.Release({content_key}));
    EXPECT_FALSE(cache.Contains(content_key));
    ASSERT_EQ(cache.Get(content_key), nullptr);
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protected data before put");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    EXPECT_FALSE(cache.Contains(content_key));
    // protect key wait timeout and check if it exist
    EXPECT_TRUE(cache.Protect({content_key}));
    EXPECT_TRUE(cache.Put(content_key, buffer, expire_time));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    EXPECT_TRUE(cache.Contains(content_key));
    ASSERT_NE(cache.Get(content_key), nullptr);
    // release key and check if it expires
    EXPECT_TRUE(cache.Release({content_key}));
    EXPECT_FALSE(cache.Contains(content_key));
    ASSERT_EQ(cache.Get(content_key), nullptr);
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protect and release key by prefix");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    EXPECT_TRUE(cache.Put(content_key, buffer, expire_time));
    // protect key wait timeout and check if it exist
    EXPECT_TRUE(cache.Protect({prefix}));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    EXPECT_TRUE(cache.Contains(content_key));
    ASSERT_NE(cache.Get(content_key), nullptr);
    // release key and check if it expires
    EXPECT_TRUE(cache.Release({prefix}));
    EXPECT_FALSE(cache.Contains(content_key));
    ASSERT_EQ(cache.Get(content_key), nullptr);
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protect and release key, put multiple keys");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    EXPECT_TRUE(cache.Put(content_key, buffer, expire_time));
    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(
          cache.Put(content_key + std::to_string(i), buffer, expire_time));
    }
    // protect key wait timeout and check if protected key exist, other expires
    EXPECT_TRUE(cache.Protect({content_key}));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    EXPECT_TRUE(cache.Contains(content_key));
    for (auto i = 0; i < 10; i++) {
      EXPECT_FALSE(cache.Contains(content_key + std::to_string(i)));
    }
    ASSERT_NE(cache.Get(content_key), nullptr);
    // release key and check if it expires
    EXPECT_TRUE(cache.Release({content_key}));
    EXPECT_FALSE(cache.Contains(content_key));
    ASSERT_EQ(cache.Get(content_key), nullptr);
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protect multiple keys in multiple calls");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(
          cache.Put(content_key + std::to_string(i), buffer, expire_time));
    }
    // protect some keys wait timeout and check if protected key exist, other
    // expires
    for (auto i = 0; i < 5; i++) {
      EXPECT_TRUE(cache.Protect({content_key + std::to_string(i)}));
    }
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    for (auto i = 0; i < 5; i++) {
      EXPECT_TRUE(cache.Contains(content_key + std::to_string(i)));
    }
    for (auto i = 5; i < 10; i++) {
      EXPECT_FALSE(cache.Contains(content_key + std::to_string(i)));
    }
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protect and release multiple keys in one call");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(
          cache.Put(content_key + std::to_string(i), buffer, expire_time));
    }
    // protect some keys wait timeout and check if protected key exist, other
    // expires
    cache::DefaultCache::KeyListType list_to_protect;
    list_to_protect.reserve(5);
    for (auto i = 0; i < 5; i++) {
      list_to_protect.push_back(content_key + std::to_string(i));
    }
    EXPECT_TRUE(cache.Protect(list_to_protect));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    for (auto i = 0; i < 5; i++) {
      EXPECT_TRUE(cache.Contains(content_key + std::to_string(i)));
    }
    for (auto i = 5; i < 10; i++) {
      EXPECT_FALSE(cache.Contains(content_key + std::to_string(i)));
    }
    // release keys
    EXPECT_TRUE(cache.Release(list_to_protect));
    for (auto i = 0; i < 5; i++) {
      EXPECT_FALSE(cache.Contains(content_key + std::to_string(i)));
    }
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protect and release multiple keys by prefix");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(
          cache.Put(content_key + std::to_string(i), buffer, expire_time));
    }
    // protect some keys wait timeout and check if protected key exist
    EXPECT_TRUE(cache.Protect({prefix}));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(cache.Contains(content_key + std::to_string(i)));
    }
    // release keys
    EXPECT_TRUE(cache.Release({prefix}));
    for (auto i = 0; i < 10; i++) {
      EXPECT_FALSE(cache.Contains(content_key + std::to_string(i)));
    }
    cache.Clear();
    cache.Close();
  }
  {
    SCOPED_TRACE("Protect and release multiple keys by prefix");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);
    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(
          cache.Put(content_key + std::to_string(i), buffer, expire_time));
    }
    // protect keys with prefix and key with the same prefix
    EXPECT_TRUE(cache.Protect({prefix, content_key}));
    std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));
    for (auto i = 0; i < 10; i++) {
      EXPECT_TRUE(cache.Contains(content_key + std::to_string(i)));
    }
    // try to release key, which protected by prefix
    EXPECT_FALSE(cache.Release({content_key}));

    cache.Clear();
    cache.Close();
  }
}
TEST(DefaultCacheTest, ProtectedLruEviction) {
  {
    SCOPED_TRACE("Protect and release keys, which suppose to be evicted");

    const auto prefix{"somekey"};
    const auto data_size = 1024u;
    std::vector<unsigned char> binary_data(data_size);
    auto data = std::make_shared<std::vector<unsigned char>>(binary_data);
    cache::CacheSettings settings;
    settings.disk_path_mutable = "./cache";
    settings.eviction_policy = cache::EvictionPolicy::kLeastRecentlyUsed;
    settings.max_disk_storage = 2u * 1024u * 1024u;
    cache::DefaultCache cache(settings);

    cache.Open();
    cache.Clear();
    // protect all keys, than close cache
    cache.Protect({prefix});
    cache.Close();
    cache.Open();

    const auto protected_key = prefix + std::to_string(0);
    const auto evicted_key = prefix + std::to_string(1);
    cache.Put(protected_key, data, (std::numeric_limits<time_t>::max)());

    // overflow the mutable cache
    auto count = 0u;
    std::string key;
    const auto max_count = settings.max_disk_storage / data_size;
    for (; count < max_count; ++count) {
      key = prefix + std::to_string(count);
      const auto result =
          cache.Put(key, data, (std::numeric_limits<time_t>::max)());

      ASSERT_TRUE(result);

      EXPECT_TRUE(cache.Contains(key));
    }

    // maximum is reached. Check if no keys was evicted
    ASSERT_TRUE(count == max_count);
    for (auto i = 0u; i < count; i++) {
      EXPECT_TRUE(cache.Contains(prefix + std::to_string(i)));
    }
    // now release keys by prefix and protect single key
    cache.Release({prefix});
    cache.Protect({protected_key});
    // put some keys to trigger eviction, even keys was promoted, we will not
    // evict protected_key
    for (auto i = 1u; i < count; i++) {
      auto some_key = prefix + std::to_string(i);
      const auto result =
          cache.Put(some_key, data, (std::numeric_limits<time_t>::max)());

      ASSERT_TRUE(result);
      EXPECT_TRUE(cache.Contains(some_key));
    }

    // key was evicted, but protected still in cache
    EXPECT_FALSE(cache.Contains(evicted_key));
    EXPECT_TRUE(cache.Contains(protected_key));
    // now release protected key and put some other keys again
    cache.Release({protected_key});

    for (auto i = 1u; i < count; i++) {
      auto some_key = prefix + std::to_string(i);
      const auto result =
          cache.Put(some_key, data, (std::numeric_limits<time_t>::max)());

      ASSERT_TRUE(result);
      EXPECT_TRUE(cache.Contains(some_key));
    }
    EXPECT_FALSE(cache.Contains(protected_key));
    cache.Clear();
    cache.Close();
  }
}

}  // namespace
