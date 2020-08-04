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
#include <chrono>

#include <cache/DefaultCacheImpl.h>
#include <olp/core/utils/Dir.h>

namespace {
namespace cache = olp::cache;
class DefaultCacheImplTest : public ::testing::Test {
  void TearDown() override { olp::utils::Dir::Remove(cache_path_); }

 protected:
  const std::string cache_path_ =
      olp::utils::Dir::TempDirectory() + "/unittest";
};

class DefaultCacheImplHelper : public cache::DefaultCacheImpl {
 public:
  explicit DefaultCacheImplHelper(const cache::CacheSettings& settings)
      : cache::DefaultCacheImpl(settings) {}

  bool HasLruCache() const { return GetMutableCacheLru().get() != nullptr; }

  bool ContainsLru(const std::string& key) const {
    const auto& lru_cache = GetMutableCacheLru();
    if (!lru_cache) {
      return false;
    }

    return lru_cache->FindNoPromote(key) != lru_cache->end();
  }

  bool ContainsMemoryCache(const std::string& key) const {
    const auto& memory_cache = GetMemoryCache();
    if (!memory_cache) {
      return false;
    }

    return !memory_cache->Get(key).empty();
  }

  bool ContainsMutableCache(const std::string& key) const {
    const auto& disk_cache = GetMutableCache();
    if (!disk_cache) {
      return false;
    }

    return disk_cache->Get(key) != boost::none;
  }

  uint64_t Size() const { return GetMutableCacheSize(); }

  uint64_t CalculateExpirySize(const std::string& key) const {
    const auto expiry_key = GetExpiryKey(key);
    const auto& disk_cache = GetMutableCache();
    if (!disk_cache) {
      return 0u;
    }

    const auto expiry_str = disk_cache->Get(expiry_key).get_value_or({});
    if (expiry_str.empty()) {
      return 0u;
    }

    return expiry_key.size() + expiry_str.size();
  }

  DiskLruCache::const_iterator BeginLru() {
    const auto& lru_cache = GetMutableCacheLru();
    if (!lru_cache) {
      return DiskLruCache::const_iterator{};
    }

    return lru_cache->begin();
  }

  DiskLruCache::const_iterator EndLru() {
    const auto& lru_cache = GetMutableCacheLru();
    if (!lru_cache) {
      return DiskLruCache::const_iterator{};
    }

    return lru_cache->end();
  }
};

TEST_F(DefaultCacheImplTest, LruCache) {
  {
    SCOPED_TRACE("Successful creation");

    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_TRUE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("kLeastRecentlyUsed eviction policy");

    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.eviction_policy = cache::EvictionPolicy::kLeastRecentlyUsed;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_TRUE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("Close");

    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Close();

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("No Open() call");

    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    DefaultCacheImplHelper cache(settings);

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("Default settings");

    cache::CacheSettings settings;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("No disk cache size limit");

    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.max_disk_storage = std::uint64_t(-1);
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("kNone eviction policy");

    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.eviction_policy = cache::EvictionPolicy::kNone;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_FALSE(cache.HasLruCache());
  }
}

TEST_F(DefaultCacheImplTest, LruCachePut) {
  cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";

  {
    SCOPED_TRACE("Put decode");

    constexpr auto data_string{"this is key's data"};
    constexpr auto key{"somekey"};
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();
    cache.Put(key, data_string, [=]() { return data_string; },
              (std::numeric_limits<time_t>::max)());

    EXPECT_TRUE(cache.ContainsLru(key));
  }

  {
    SCOPED_TRACE("Put binary");

    std::vector<unsigned char> binary_data = {1, 2, 3};
    constexpr auto key{"somekey"};
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();
    cache.Put(key, std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());

    EXPECT_TRUE(cache.ContainsLru(key));
  }
}

TEST_F(DefaultCacheImplTest, LruCacheGetPromote) {
  cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  constexpr auto key1{"somekey1"};
  constexpr auto key2{"somekey2"};
  constexpr auto data_string{"this is key's data"};
  std::vector<unsigned char> binary_data = {1, 2, 3};

  DefaultCacheImplHelper cache(settings);

  cache.Open();
  cache.Clear();
  cache.Put(key1, data_string, [=]() { return data_string; },
            (std::numeric_limits<time_t>::max)());
  cache.Put(key2, std::make_shared<std::vector<unsigned char>>(binary_data),
            (std::numeric_limits<time_t>::max)());

  {
    SCOPED_TRACE("Get decode promote");

    cache.Get(key1, [=](const std::string&) { return data_string; });
    auto it = cache.BeginLru();

    EXPECT_EQ(key1, it->key());
  }

  {
    SCOPED_TRACE("Get binary promote");

    cache.Get(key2);
    auto it = cache.BeginLru();

    EXPECT_EQ(key2, it->key());
  }
}

TEST_F(DefaultCacheImplTest, LruCacheRemove) {
  cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  constexpr auto key1{"somekey1"};
  constexpr auto key2{"somekey2"};
  constexpr auto key3{"anotherkey1"};
  constexpr auto invalid_key{"invalid"};
  constexpr auto data_string{"this is key's data"};
  std::vector<unsigned char> binary_data = {1, 2, 3};

  {
    SCOPED_TRACE("Remove from cache");

    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    bool put_result = cache.Put(
        key1, std::make_shared<std::vector<unsigned char>>(binary_data),
        (std::numeric_limits<time_t>::max)());
    ASSERT_TRUE(put_result);

    auto data_read = cache.Get(key1);

    ASSERT_TRUE(data_read);
    EXPECT_EQ(*data_read, binary_data);

    // check removing missing key is not an error.
    EXPECT_TRUE(cache.Remove(invalid_key));
    EXPECT_TRUE(cache.Remove(key1));
    EXPECT_FALSE(cache.ContainsLru(key1));
  }

  {
    SCOPED_TRACE("RemoveWithPrefix");

    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());
    cache.Put(key2, std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());
    cache.Put(key3, data_string, [=]() { return data_string; },
              (std::numeric_limits<time_t>::max)());

    EXPECT_TRUE(cache.ContainsLru(key1));
    EXPECT_TRUE(cache.ContainsLru(key2));
    EXPECT_TRUE(cache.ContainsLru(key3));

    auto result = cache.RemoveKeysWithPrefix(invalid_key);

    ASSERT_TRUE(result);
    EXPECT_TRUE(cache.ContainsLru(key1));
    EXPECT_TRUE(cache.ContainsLru(key2));
    EXPECT_TRUE(cache.ContainsLru(key3));

    result = cache.RemoveKeysWithPrefix("another");

    ASSERT_TRUE(result);
    EXPECT_TRUE(cache.ContainsLru(key1));
    EXPECT_TRUE(cache.ContainsLru(key2));
    EXPECT_FALSE(cache.ContainsLru(key3));

    result = cache.RemoveKeysWithPrefix("some");

    ASSERT_TRUE(result);
    EXPECT_FALSE(cache.ContainsLru(key1));
    EXPECT_FALSE(cache.ContainsLru(key2));
  }
}
TEST_F(DefaultCacheImplTest, MutableCacheExpired) {
  SCOPED_TRACE("Expiry mutable cache");
  const std::string key1{"somekey1"};
  const std::string key2{"somekey2"};
  const std::string data_string{"this is key's data"};
  constexpr auto expiry = 2;
  std::vector<unsigned char> binary_data = {1, 2, 3};
  const auto data_ptr =
      std::make_shared<std::vector<unsigned char>>(binary_data);

  cache::CacheSettings settings1;
  settings1.disk_path_mutable = cache_path_;
  DefaultCacheImplHelper cache1(settings1);
  cache1.Open();
  cache1.Clear();

  cache1.Put(key1, data_ptr, expiry);
  cache1.Put(key2, data_string, [=]() { return data_string; }, expiry);
  cache1.Close();

  cache::CacheSettings settings2;
  settings2.disk_path_mutable = cache_path_;
  DefaultCacheImplHelper cache2(settings2);
  cache2.Open();

  const auto value = cache2.Get(key1);
  const auto value2 =
      cache2.Get(key2, [](const std::string& value) { return value; });

  EXPECT_FALSE(value.get() == nullptr);
  EXPECT_EQ(value2.type(), typeid(std::string));
  auto str = boost::any_cast<std::string>(value2);
  EXPECT_EQ(str, data_string);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  EXPECT_TRUE(cache2.Get(key1).get() == nullptr);
  EXPECT_TRUE(
      cache2.Get(key2, [](const std::string& value) { return value; }).empty());
  cache2.Close();
}

TEST_F(DefaultCacheImplTest, ProtectedCacheExpired) {
  SCOPED_TRACE("Expiry protected cache");
  const std::string key1{"somekey1"};
  const std::string key2{"somekey2"};
  const std::string data_string{"this is key's data"};
  constexpr auto expiry = 2;
  std::vector<unsigned char> binary_data = {1, 2, 3};
  const auto data_ptr =
      std::make_shared<std::vector<unsigned char>>(binary_data);

  cache::CacheSettings settings1;
  settings1.disk_path_mutable = cache_path_;
  DefaultCacheImplHelper cache1(settings1);
  cache1.Open();
  cache1.Clear();

  cache1.Put(key1, data_ptr, expiry);
  cache1.Put(key2, data_string, [=]() { return data_string; }, expiry);
  cache1.Close();

  cache::CacheSettings settings2;
  settings2.disk_path_protected = cache_path_;
  DefaultCacheImplHelper cache2(settings2);
  cache2.Open();

  const auto value = cache2.Get(key1);
  const auto value2 =
      cache2.Get(key2, [](const std::string& value) { return value; });

  EXPECT_FALSE(value.get() == nullptr);
  EXPECT_EQ(value2.type(), typeid(std::string));
  auto str = boost::any_cast<std::string>(value2);
  EXPECT_EQ(str, data_string);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  EXPECT_TRUE(cache2.Get(key1).get() == nullptr);
  EXPECT_TRUE(
      cache2.Get(key2, [](const std::string& value) { return value; }).empty());
  cache2.Close();
}

TEST_F(DefaultCacheImplTest, MutableCacheSize) {
  const std::string key1{"somekey1"};
  const std::string key2{"somekey2"};
  const std::string key3{"anotherkey1"};
  const std::string invalid_key{"invalid"};
  const std::string data_string{"this is key's data"};
  constexpr auto expiry = 321;
  std::vector<unsigned char> binary_data = {1, 2, 3};
  const auto data_ptr =
      std::make_shared<std::vector<unsigned char>>(binary_data);

  cache::CacheSettings settings;
  settings.disk_path_mutable = cache_path_;

  {
    SCOPED_TRACE("Put decode");

    settings.eviction_policy = cache::EvictionPolicy::kNone;
    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, data_string, [=]() { return data_string; },
              (std::numeric_limits<time_t>::max)());
    auto data_size = key1.size() + data_string.size();

    EXPECT_EQ(data_size, cache.Size());

    cache.Put(key2, data_string, [=]() { return data_string; }, expiry);
    data_size +=
        key2.size() + data_string.size() + cache.CalculateExpirySize(key2);

    EXPECT_EQ(data_size, cache.Size());
  }

  {
    SCOPED_TRACE("Put binary");

    settings.eviction_policy = cache::EvictionPolicy::kNone;
    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, data_ptr, (std::numeric_limits<time_t>::max)());
    auto data_size = key1.size() + binary_data.size();

    EXPECT_EQ(data_size, cache.Size());

    cache.Put(key2, data_ptr, expiry);
    data_size +=
        key2.size() + binary_data.size() + cache.CalculateExpirySize(key2);

    EXPECT_EQ(data_size, cache.Size());
  }

  {
    SCOPED_TRACE("Remove from cache");

    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, data_ptr, (std::numeric_limits<time_t>::max)());
    cache.Put(key2, data_string, [=]() { return data_string; },
              (std::numeric_limits<time_t>::max)());

    cache.Remove(key1);
    cache.Remove(key2);
    cache.Remove(invalid_key);

    EXPECT_EQ(0u, cache.Size());
  }

  {
    SCOPED_TRACE("Remove from cache with expiry");

    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, data_ptr, expiry);
    cache.Put(key2, data_string, [=]() { return data_string; }, expiry);

    cache.Remove(key1);
    cache.Remove(key2);
    cache.Remove(invalid_key);

    EXPECT_EQ(0u, cache.Size());
  }

  {
    SCOPED_TRACE("RemoveWithPrefix");

    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, data_ptr, (std::numeric_limits<time_t>::max)());
    cache.Put(key2, data_ptr, expiry);
    cache.Put(key3, data_string, [=]() { return data_string; }, expiry);
    const auto data_size =
        key3.size() + data_string.size() + cache.CalculateExpirySize(key3);

    cache.RemoveKeysWithPrefix(invalid_key);
    cache.RemoveKeysWithPrefix("some");

    EXPECT_EQ(data_size, cache.Size());
  }

  {
    SCOPED_TRACE("Expiry");

    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, data_ptr, -1);
    cache.Put(key2, data_string, [=]() { return data_string; }, -1);

    cache.Get(key1);
    cache.Get(key2, [](const std::string& value) { return value; });

    EXPECT_EQ(0, cache.Size());
  }

  {
    SCOPED_TRACE("Clear/close/open");
    const std::string data_string{"this is key's data"};
    const std::string key{"somekey"};

    settings.eviction_policy = cache::EvictionPolicy::kNone;
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();

    cache.Put(key, data_string, [=]() { return data_string; },
              (std::numeric_limits<time_t>::max)());
    const auto data_size = key.size() + data_string.size();
    cache.Close();
    EXPECT_EQ(0u, cache.Size());

    cache.Open();
    EXPECT_EQ(data_size, cache.Size());

    cache.Clear();
    EXPECT_EQ(0u, cache.Size());
  }

  {
    SCOPED_TRACE("Cache not blocked");

    const auto prefix = "somekey";
    const auto data_size = 1024u;
    std::vector<unsigned char> binary_data(data_size);
    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.eviction_policy = cache::EvictionPolicy::kNone;
    settings.max_disk_storage = 2u * 1024u * 1024u;
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();

    // fill the cache till it full
    auto count = 0u;
    const auto max_count = settings.max_disk_storage / data_size;
    auto total_size = 0u;
    for (; count < max_count; ++count) {
      const auto key = prefix + std::to_string(count);
      const auto elem_size = key.size() + binary_data.size();
      if (total_size + elem_size > settings.max_disk_storage) {
        break;
      }

      const auto result = cache.Put(
          key, std::make_shared<std::vector<unsigned char>>(binary_data),
          (std::numeric_limits<time_t>::max)());
      total_size += elem_size;
      ASSERT_TRUE(result);
    }

    auto result =
        cache.Put(prefix + std::to_string(count),
                  std::make_shared<std::vector<unsigned char>>(binary_data),
                  (std::numeric_limits<time_t>::max)());
    EXPECT_FALSE(result);
    EXPECT_TRUE(total_size < settings.max_disk_storage);
    EXPECT_EQ(total_size, cache.Size());

    cache.Remove(prefix + std::to_string(count - 1));
    cache.Remove(prefix + std::to_string(count - 2));
    result =
        cache.Put(prefix + std::to_string(count),
                  std::make_shared<std::vector<unsigned char>>(binary_data),
                  (std::numeric_limits<time_t>::max)());

    EXPECT_TRUE(result);
    EXPECT_TRUE(total_size > cache.Size());
  }
}

TEST_F(DefaultCacheImplTest, LruCacheEviction) {
  {
    SCOPED_TRACE("kNone evicts nothing");

    const auto prefix = "somekey";
    const auto data_size = 1024u;
    std::vector<unsigned char> binary_data(data_size);
    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.eviction_policy = cache::EvictionPolicy::kNone;
    settings.max_disk_storage = 2u * 1024u * 1024u;
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();

    // fill the cache till it full
    auto count = 0u;
    const auto max_count = settings.max_disk_storage / data_size;
    for (; count < max_count; ++count) {
      const auto key = prefix + std::to_string(count);
      const auto result = cache.Put(
          key, std::make_shared<std::vector<unsigned char>>(binary_data),
          (std::numeric_limits<time_t>::max)());
      if (!result) {
        break;
      }

      EXPECT_TRUE(cache.ContainsMutableCache(key));
      EXPECT_TRUE(cache.ContainsMemoryCache(key));
    }

    // maximum is reached = cache to full.
    ASSERT_FALSE(count == max_count);
    EXPECT_FALSE(cache.HasLruCache());

    // check all data is in the cache.
    for (auto i = 0u; i < count; ++i) {
      const auto key = prefix + std::to_string(i);
      const auto value = cache.Get(key);

      EXPECT_TRUE(value.get() != nullptr);
      EXPECT_TRUE(cache.ContainsMutableCache(key));
      EXPECT_TRUE(cache.ContainsMemoryCache(key));
    }
    cache.Clear();
  }

  {
    SCOPED_TRACE("kLeastRecentlyUsed eviction, default expiry");

    const auto prefix{"somekey"};
    const auto data_size = 1024u;
    std::vector<unsigned char> binary_data(data_size);
    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.eviction_policy = cache::EvictionPolicy::kLeastRecentlyUsed;
    settings.max_disk_storage = 2u * 1024u * 1024u;
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();

    const auto promote_key = prefix + std::to_string(0);
    const auto evicted_key = prefix + std::to_string(1);
    cache.Put(promote_key,
              std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());

    // overflow the mutable cache
    auto count = 0u;
    std::string key;
    const auto max_count = settings.max_disk_storage / data_size;
    for (; count < max_count; ++count) {
      key = prefix + std::to_string(count);
      const auto result = cache.Put(
          key, std::make_shared<std::vector<unsigned char>>(binary_data),
          (std::numeric_limits<time_t>::max)());

      ASSERT_TRUE(result);

      // promote the key so its not evicted.
      cache.Get(promote_key);

      EXPECT_TRUE(cache.ContainsMutableCache(key));
      EXPECT_TRUE(cache.ContainsMemoryCache(key));
      EXPECT_TRUE(cache.ContainsLru(key));
      EXPECT_TRUE(cache.ContainsMutableCache(promote_key));
      EXPECT_TRUE(cache.ContainsMemoryCache(promote_key));
      EXPECT_TRUE(cache.ContainsLru(promote_key));
    }
    const auto promote_value = cache.Get(promote_key);
    const auto value = cache.Get(key);

    // maximum is reached.
    ASSERT_TRUE(count == max_count);
    EXPECT_TRUE(cache.HasLruCache());
    EXPECT_TRUE(value.get() != nullptr);
    EXPECT_TRUE(promote_value.get() != nullptr);

    EXPECT_TRUE(cache.ContainsMutableCache(promote_key));
    EXPECT_TRUE(cache.ContainsMemoryCache(promote_key));
    EXPECT_TRUE(cache.ContainsLru(promote_key));
    EXPECT_TRUE(cache.ContainsMutableCache(key));
    EXPECT_TRUE(cache.ContainsMemoryCache(key));
    EXPECT_TRUE(cache.ContainsLru(key));

    // some items are removed, because eviction starts before the cache is full
    EXPECT_FALSE(cache.ContainsMutableCache(evicted_key));
    EXPECT_FALSE(cache.ContainsMemoryCache(evicted_key));
    EXPECT_FALSE(cache.ContainsLru(evicted_key));
    cache.Clear();
  }
  {
    SCOPED_TRACE("kLeastRecentlyUsed eviction, expired removed first");

    const auto prefix{"somekey"};
    const auto data_size = 1024u;
    std::vector<unsigned char> binary_data(data_size);
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.eviction_policy = cache::EvictionPolicy::kLeastRecentlyUsed;
    settings.max_disk_storage = 2u * 1024u * 1024u;
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();

    EXPECT_TRUE(cache.HasLruCache());

    const auto not_expired_key = prefix + std::to_string(0);

    // Put data that expires after 10 sec and never promoted.
    cache.Put(not_expired_key,
              std::make_shared<std::vector<unsigned char>>(binary_data), 10);

    // overflow the mutable cache
    auto count = 1u;
    std::string key;
    const auto max_count = settings.max_disk_storage / data_size;
    for (; count < max_count; ++count) {
      key = prefix + std::to_string(count);

      // Put data that is already expired.
      const auto result = cache.Put(
          key, std::make_shared<std::vector<unsigned char>>(binary_data), -1);

      ASSERT_TRUE(result);

      // Expect that not yet expired key is always present
      EXPECT_TRUE(cache.ContainsMutableCache(not_expired_key));
      EXPECT_TRUE(cache.ContainsLru(not_expired_key));
    }
    const auto not_expired_value = cache.Get(not_expired_key);
    EXPECT_TRUE(not_expired_value.get() != nullptr);

    cache.Clear();
  }
}

TEST_F(DefaultCacheImplTest, ProtectTest) {
  const std::string key1_data_string = "this is key1's data";
  const std::string key2_data_string = "this is key2's data";
  const std::string key1 = "key1";
  const std::string key2 = "key2";
  {
    SCOPED_TRACE("Check key exist cache with lru, store expired");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    settings.max_memory_cache_size = 0;
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Protect({key1}));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(cache.Contains(key1));
    // check if it is really in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check key exist cache with memory cache, store expired");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Protect({key1}));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(cache.Contains(key1));
    // check if it is really in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE(
        "Check key exist cache with memory cache, protect before store "
        "expired");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    ASSERT_TRUE(cache.Protect({key1}));
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(cache.Contains(key1));
    // check if it is really in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check if key still protected exist after closing");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    {
      DefaultCacheImplHelper cache(settings);
      ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
      ASSERT_TRUE(cache.Clear());
      ASSERT_TRUE(cache.Protect({key1}));
      cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    }
    {
      DefaultCacheImplHelper cache(settings);
      ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
      cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
      std::this_thread::sleep_for(std::chrono::seconds(3));
      ASSERT_TRUE(cache.Contains(key1));
      // check if it didn't expire
      auto key1_data_read =
          cache.Get(key1, [](const std::string& data) { return data; });
      ASSERT_FALSE(key1_data_read.empty());
      ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
      ASSERT_TRUE(cache.Clear());
    }
  }
  {
    SCOPED_TRACE(
        "Check if key could be released, expiration finished before release");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Protect({key1}));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(cache.Contains(key1));
    // check if it is really in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    // after release try to check again
    ASSERT_TRUE(cache.Release({key1}));
    ASSERT_FALSE(cache.Contains(key1));
    auto released_key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_TRUE(released_key1_data_read.empty());
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE(
        "Check if key could be released, expiration finished after release ");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Protect({key1}));
    // check if it is really in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    // after release try to check again
    ASSERT_TRUE(cache.Release({key1}));
    ASSERT_TRUE(cache.Contains(key1));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_FALSE(cache.Contains(key1));
    auto released_key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_TRUE(released_key1_data_read.empty());
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check if key could be released via prefix ");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Protect({key1}));
    // check if it is really in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    // after release try to check again
    ASSERT_TRUE(cache.Release({"k"}));
    ASSERT_TRUE(cache.Contains(key1));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_FALSE(cache.Contains(key1));
    auto released_key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_TRUE(released_key1_data_read.empty());
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Check if key could be protected and released via prefix ");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Protect({"k"}));
    // check if it is really in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    // after release try to check again
    ASSERT_TRUE(cache.Release({"k"}));
    ASSERT_TRUE(cache.Contains(key1));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_FALSE(cache.Contains(key1));
    auto released_key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_TRUE(released_key1_data_read.empty());
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE(
        "Check if multiple keys could be protected and released via prefix ");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    cache.Put(key2, key2_data_string, [=]() { return key2_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key2));
    ASSERT_TRUE(cache.Protect({key1, key2, "key3", "key"}));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Contains(key2));

    // check if it is in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    // after release try to check again
    ASSERT_TRUE(cache.Release({"k"}));
    ASSERT_FALSE(cache.Contains(key1));
    ASSERT_FALSE(cache.Contains(key2));
    auto released_key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_TRUE(released_key1_data_read.empty());
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Try to protect key already protected by prefix");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    cache.Put(key2, key2_data_string, [=]() { return key2_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key2));
    ASSERT_TRUE(cache.Protect({"key"}));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_FALSE(cache.Protect({key1, key2}));
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Contains(key2));

    // check if it is in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    // after release key, which is protected by prefix
    // we can not release one key, if it is protected by prefix
    ASSERT_FALSE(cache.Release({key1}));
    // release failed, keys still in cache
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_TRUE(cache.Contains(key2));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Try to protect keys, release one key prefix");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    ASSERT_TRUE(cache.Protect({"key", "other_key1", "other_key2"}));
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    ASSERT_TRUE(cache.Contains(key1));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(cache.Contains(key1));

    // check if it is in cache
    auto key1_data_read =
        cache.Get(key1, [](const std::string& data) { return data; });
    ASSERT_FALSE(key1_data_read.empty());
    ASSERT_EQ(key1_data_string, boost::any_cast<std::string>(key1_data_read));
    // after release key prefix
    ASSERT_TRUE(cache.Release({"key"}));
    ASSERT_FALSE(cache.Contains(key1));
    ASSERT_TRUE(cache.Clear());
  }
  {
    SCOPED_TRACE("Try to protect already protected key");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    ASSERT_TRUE(cache.Protect({key1, key2}));
    ASSERT_FALSE(cache.Protect({key1, key2}));
    // after try to release key twice
    ASSERT_TRUE(cache.Release({key1, key2}));
    ASSERT_FALSE(cache.Release({key1, key2}));
    ASSERT_TRUE(cache.Clear());
  }
}

TEST_F(DefaultCacheImplTest, LruCacheEvictionWithProtected) {
  {
    SCOPED_TRACE("Protect and release keys, which suppose to be evicted");

    const auto prefix{"somekey"};
    const auto internal_key{"internal::protected::protected_data"};
    const auto data_size = 1024u;
    std::vector<unsigned char> binary_data(data_size);
    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    settings.eviction_policy = cache::EvictionPolicy::kLeastRecentlyUsed;
    settings.max_disk_storage = 2u * 1024u * 1024u;
    DefaultCacheImplHelper cache(settings);

    cache.Open();
    cache.Clear();
    // protect all keys
    cache.Protect({prefix});
    cache.Close();
    cache.Open();
    // check if after Open internal key is not in lru
    EXPECT_FALSE(cache.ContainsLru(internal_key));

    const auto promote_key = prefix + std::to_string(0);
    const auto evicted_key = prefix + std::to_string(1);
    cache.Put(promote_key,
              std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());

    // overflow the mutable cache
    auto count = 0u;
    std::string key;
    const auto max_count = settings.max_disk_storage / data_size;
    for (; count < max_count; ++count) {
      key = prefix + std::to_string(count);
      const auto result = cache.Put(
          key, std::make_shared<std::vector<unsigned char>>(binary_data),
          (std::numeric_limits<time_t>::max)());

      ASSERT_TRUE(result);

      EXPECT_TRUE(cache.ContainsMutableCache(key));
      EXPECT_TRUE(cache.ContainsMemoryCache(key));
    }

    // maximum is reached.
    ASSERT_TRUE(count == max_count);
    EXPECT_TRUE(cache.HasLruCache());
    EXPECT_TRUE(cache.ContainsMutableCache(internal_key));
    EXPECT_FALSE(cache.ContainsLru(internal_key));

    // no keys was evicted
    EXPECT_TRUE(cache.ContainsMutableCache(evicted_key));
    cache.Release({prefix});
    cache.Get(promote_key);
    cache.Put(promote_key,
              std::make_shared<std::vector<unsigned char>>(binary_data),
              (std::numeric_limits<time_t>::max)());
    // mutable cache updated
    EXPECT_FALSE(cache.ContainsMutableCache(evicted_key));
    cache.Clear();
  }
}
}  // namespace
