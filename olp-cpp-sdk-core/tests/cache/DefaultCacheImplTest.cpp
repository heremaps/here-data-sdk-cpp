/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <cache/DefaultCacheImpl.h>
#include <olp/core/utils/Dir.h>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef max
#endif

#include "Helpers.h"

namespace {
namespace cache = olp::cache;
using CacheType = cache::DefaultCache::CacheType;

bool SetRights(const std::string& path, bool readonly) {
#if !defined(_WIN32) || defined(__MINGW32__)
  mode_t mode;
  if (readonly) {
    mode = S_IRUSR | S_IRGRP | S_IROTH;
  } else {
    mode = S_IRUSR | S_IWUSR | S_IXUSR;
  }
  mode = mode | S_IRGRP | S_IROTH;
  return chmod(path.c_str(), mode) == 0;
#else
  DWORD attributes = GetFileAttributes(path.c_str());
  attributes = readonly ? attributes | FILE_ATTRIBUTE_READONLY
                        : attributes & ~FILE_ATTRIBUTE_READONLY;
  BOOL result = SetFileAttributesA(path.c_str(), attributes);
  return result != 0;
#endif
}

class DefaultCacheImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Restore permissions in case if cache_path_ is not writable
    helpers::MakeDirectoryAndContentReadonly(cache_path_, false);
  }

  void TearDown() override { olp::utils::Dir::Remove(cache_path_); }

  uint64_t GetCacheSizeOnDisk() {
    const std::string ldb_ext = ".ldb";
    return olp::utils::Dir::Size(cache_path_, [&](const std::string& path) {
      // Taking into account only ldb files.
      // Other files: lock, logs, manifest are quite small (~100KB on >1GB db)
      // but on a compacted cache they could take more than a few megabytes
      if (path.length() <= ldb_ext.length()) {
        return false;
      }
      return path.substr(path.length() - ldb_ext.length()) == ldb_ext;
    });
  }

 protected:
  const std::string cache_path_ =
      olp::utils::Dir::TempDirectory() + "/unittest";
};

class DefaultCacheImplHelper : public cache::DefaultCacheImpl {
 public:
  explicit DefaultCacheImplHelper(const cache::CacheSettings& settings)
      : cache::DefaultCacheImpl(settings) {}

  bool HasLruCache() const { return GetMutableCacheLru().get() != nullptr; }
  bool HasMutableCache() const {
    return GetCache(CacheType::kMutable).get() != nullptr;
  }
  bool HasProtectedCache() const {
    return GetCache(CacheType::kProtected).get() != nullptr;
  }

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
    const auto& disk_cache = GetCache(CacheType::kMutable);
    if (!disk_cache) {
      return false;
    }

    return disk_cache->Get(key).IsSuccessful();
  }

  uint64_t CalculateExpirySize(const std::string& key) const {
    const auto expiry_key = GetExpiryKey(key);
    const auto& disk_cache = GetCache(CacheType::kMutable);
    if (!disk_cache) {
      return 0u;
    }

    auto expiry_result = disk_cache->Get(expiry_key);
    if (!expiry_result) {
      return 0u;
    }
    std::string expiry_str(expiry_result.GetResult()->begin(),
                           expiry_result.GetResult()->end());
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

  void SetEvictionPortion(uint64_t size) {
    cache::DefaultCacheImpl::SetEvictionPortion(size);
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
    EXPECT_EQ(key1, cache.BeginLru()->key());
  }

  {
    SCOPED_TRACE("Get binary promote");

    cache.Get(key2);
    EXPECT_EQ(key2, cache.BeginLru()->key());
  }

  {
    SCOPED_TRACE("Promote");

    cache.Promote(key1);
    EXPECT_EQ(key1, cache.BeginLru()->key());
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

  ASSERT_NE(value.get(), nullptr);
  ASSERT_EQ(value2.type(), typeid(std::string));
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

    EXPECT_EQ(data_size, cache.Size(CacheType::kMutable));

    cache.Put(key2, data_string, [=]() { return data_string; }, expiry);
    data_size +=
        key2.size() + data_string.size() + cache.CalculateExpirySize(key2);

    EXPECT_EQ(data_size, cache.Size(CacheType::kMutable));
  }

  {
    SCOPED_TRACE("Put binary");

    settings.eviction_policy = cache::EvictionPolicy::kNone;
    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Clear();

    cache.Put(key1, data_ptr, (std::numeric_limits<time_t>::max)());
    auto data_size = key1.size() + binary_data.size();

    EXPECT_EQ(data_size, cache.Size(CacheType::kMutable));

    cache.Put(key2, data_ptr, expiry);
    data_size +=
        key2.size() + binary_data.size() + cache.CalculateExpirySize(key2);

    EXPECT_EQ(data_size, cache.Size(CacheType::kMutable));
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

    EXPECT_EQ(0u, cache.Size(CacheType::kMutable));
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

    EXPECT_EQ(0u, cache.Size(CacheType::kMutable));
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

    EXPECT_EQ(data_size, cache.Size(CacheType::kMutable));
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

    EXPECT_EQ(0, cache.Size(CacheType::kMutable));
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
    cache.Close();
    EXPECT_EQ(0u, cache.Size(CacheType::kMutable));

    cache.Open();
    EXPECT_EQ(GetCacheSizeOnDisk(), cache.Size(CacheType::kMutable));

    cache.Clear();
    EXPECT_EQ(0u, cache.Size(CacheType::kMutable));
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
    EXPECT_EQ(total_size, cache.Size(CacheType::kMutable));

    cache.Remove(prefix + std::to_string(count - 1));
    cache.Remove(prefix + std::to_string(count - 2));
    result =
        cache.Put(prefix + std::to_string(count),
                  std::make_shared<std::vector<unsigned char>>(binary_data),
                  (std::numeric_limits<time_t>::max)());

    EXPECT_TRUE(result);
    EXPECT_TRUE(total_size > cache.Size(CacheType::kMutable));
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
      cache.Promote(promote_key);

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
  const std::string key3_data_string = "this is key3's data";
  const std::string key1 = "key1";
  const std::string key2 = "key2";
  const std::string key3 = "key3";
  const std::string other_key1 = "other::key1";
  const std::string other_key2 = "other::key2";
  const std::string other_key3 = "other::key3";
  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
  {
    SCOPED_TRACE("Protect keys");

    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; }, 2);
    cache.Put(other_key1, key1_data_string, [=]() { return key1_data_string; },
              2);
    ASSERT_TRUE(cache.Contains(key1));
    // protect single keys, and prefix
    ASSERT_TRUE(cache.Protect({key1, key2, "other"}));
    // Try to protect key already protected by prefix
    ASSERT_FALSE(cache.Protect({other_key1}));
    ASSERT_FALSE(cache.Contains(key2));
    cache.Put(key2, key2_data_string, [=]() { return key2_data_string; }, 2);
    cache.Put(key3, key3_data_string, [=]() { return key3_data_string; }, 2);
    cache.Put(other_key2, key2_data_string, [=]() { return key2_data_string; },
              2);
    cache.Put(other_key3, key3_data_string, [=]() { return key3_data_string; },
              2);
    ASSERT_TRUE(cache.Protect({key3}));
    ASSERT_TRUE(cache.Release({key1}));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_FALSE(cache.Contains(key1));
    ASSERT_TRUE(cache.Contains(key2));
    ASSERT_TRUE(cache.Contains(key3));
    ASSERT_TRUE(cache.Contains(other_key1));
    ASSERT_TRUE(cache.Contains(other_key2));
    ASSERT_TRUE(cache.Contains(other_key3));

    // check if it is really in cache
    auto key2_data_read =
        cache.Get(key2, [](const std::string& data) { return data; });
    ASSERT_FALSE(key2_data_read.empty());
    ASSERT_EQ(key2_data_string, boost::any_cast<std::string>(key2_data_read));
  }
  {
    SCOPED_TRACE("Check if key still protected exist after closing");
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    auto key2_data_read =
        cache.Get(key2, [](const std::string& data) { return data; });
    ASSERT_FALSE(key2_data_read.empty());
    ASSERT_EQ(key2_data_string, boost::any_cast<std::string>(key2_data_read));
    ASSERT_TRUE(cache.IsProtected(key2));
    ASSERT_TRUE(cache.IsProtected(key3));
    // key which do not exist
    ASSERT_TRUE(cache.IsProtected("other::key4"));

    // release by prefix, keys which was protected as single keys
    ASSERT_TRUE(cache.Release({"key"}));
    ASSERT_FALSE(cache.Contains(key2));
    ASSERT_FALSE(cache.Contains(key3));
    ASSERT_FALSE(cache.IsProtected(key2));
    ASSERT_FALSE(cache.IsProtected(key3));
    ASSERT_TRUE(cache.Contains(other_key1));
    ASSERT_TRUE(cache.Contains(other_key2));
    ASSERT_TRUE(cache.Contains(other_key3));
    // cannot release single keys protected by prefix
    ASSERT_FALSE(cache.Release({other_key1, other_key2}));
    ASSERT_TRUE(cache.IsProtected(other_key1));
    ASSERT_TRUE(cache.IsProtected(other_key2));
    ASSERT_TRUE(cache.Release({"other"}));
    ASSERT_FALSE(cache.IsProtected(other_key1));
    ASSERT_FALSE(cache.IsProtected(other_key2));
    ASSERT_FALSE(cache.IsProtected(other_key3));
    ASSERT_FALSE(cache.Contains(other_key1));
    ASSERT_FALSE(cache.Contains(other_key2));
    ASSERT_FALSE(cache.Contains(other_key3));
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

TEST_F(DefaultCacheImplTest, InternalKeysBypassLru) {
  {
    SCOPED_TRACE("Protect and release keys, which suppose to be evicted");

    const auto internal_key{"internal::protected::protected_data"};
    const auto data_string{"this is key's data"};
    cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path_;
    {
      settings.eviction_policy = cache::EvictionPolicy::kNone;
      DefaultCacheImplHelper cache(settings);
      ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
      cache.Clear();

      constexpr auto expiry = 2;
      EXPECT_TRUE(cache.Put(internal_key, data_string,
                            [data_string]() { return data_string; }, expiry));
    }

    settings.disk_path_mutable = cache_path_;
    settings.disk_path_protected.reset();
    settings.eviction_policy = cache::EvictionPolicy::kLeastRecentlyUsed;

    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    // no keys was evicted
    auto stored_dats = cache.Get(internal_key);
    ASSERT_TRUE(stored_dats);
    std::string stored_string(
        reinterpret_cast<const char*>(stored_dats->data()),
        stored_dats->size());
    EXPECT_EQ(stored_string, data_string);
    cache.Clear();
  }
}

TEST_F(DefaultCacheImplTest, ReadOnlyPartitionForProtectedCache) {
  SCOPED_TRACE("Read only partition protected cache");
  const std::string key{"somekey"};
  const std::string data_string{"this is key's data"};

  cache::CacheSettings settings1;
  settings1.disk_path_mutable = cache_path_;
  DefaultCacheImplHelper cache1(settings1);
  ASSERT_EQ(olp::cache::DefaultCache::StorageOpenResult::Success,
            cache1.Open());
  cache1.Clear();

  cache1.Put(key, data_string, [=]() { return data_string; },
             std::numeric_limits<time_t>::max());
  cache1.Close();

  // Make readonly
  ASSERT_TRUE(helpers::MakeDirectoryAndContentReadonly(cache_path_, true));

  cache::CacheSettings settings2;
  settings2.disk_path_protected = cache_path_;
  DefaultCacheImplHelper cache2(settings2);
  ASSERT_EQ(olp::cache::DefaultCache::StorageOpenResult::Success,
            cache2.Open());

  const auto value = cache2.Get(key);
  EXPECT_NE(nullptr, value.get());
  cache2.Close();
  helpers::MakeDirectoryAndContentReadonly(cache_path_, false);
}

TEST_F(DefaultCacheImplTest, ProtectTestWithoutMutableCache) {
  const std::string key1_data_string = "this is key1's data";
  const std::string key1 = "key1";

  {
    SCOPED_TRACE("Protect key in memory cache");
    DefaultCacheImplHelper cache({});
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    cache.Put(key1, key1_data_string, [=]() { return key1_data_string; },
              std::numeric_limits<time_t>::max());
    ASSERT_TRUE(cache.Contains(key1));
    ASSERT_FALSE(cache.Protect({key1}));

    cache.Close();
  }
  {
    SCOPED_TRACE("Write mutable cache");
    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = olp::utils::Dir::TempDirectory() + "/unittest";
    settings.max_memory_cache_size = 0;
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_TRUE(cache.Clear());
    ASSERT_TRUE(cache.Put(key1, key1_data_string,
                          [=]() { return key1_data_string; },
                          std::numeric_limits<time_t>::max()));
    ASSERT_TRUE(cache.Protect({key1}));
    cache.Close();
  }
  {
    SCOPED_TRACE("Protect key in protected cache");
    olp::cache::CacheSettings settings;
    settings.disk_path_protected =
        olp::utils::Dir::TempDirectory() + "/unittest";
    settings.max_memory_cache_size = 0;
    DefaultCacheImplHelper cache(settings);
    ASSERT_EQ(olp::cache::DefaultCache::Success, cache.Open());
    ASSERT_FALSE(cache.Release({key1}));
    ASSERT_FALSE(cache.Protect({key1}));
    cache.Close();
    ASSERT_TRUE(olp::utils::Dir::Remove(settings.disk_path_protected.get()));
  }
}

TEST_F(DefaultCacheImplTest, OpenTypeCache) {
  cache::CacheSettings settings;
  settings.disk_path_mutable = cache_path_;

  {
    SCOPED_TRACE("Correct usage, mutable");

    DefaultCacheImplHelper cache(settings);
    cache.Open();

    ASSERT_TRUE(cache.HasLruCache());
    ASSERT_TRUE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());

    const auto is_closed = cache.Close(CacheType::kMutable);

    ASSERT_TRUE(is_closed);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());

    const auto result = cache.Open(CacheType::kMutable);

    ASSERT_EQ(result, cache::DefaultCache::Success);
    ASSERT_TRUE(cache.HasLruCache());
    ASSERT_TRUE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());
  }

  {
    SCOPED_TRACE("Correct usage, protected");

    cache::CacheSettings protected_settings;
    protected_settings.disk_path_protected = cache_path_;
    DefaultCacheImplHelper cache(protected_settings);
    cache.Open();

    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_TRUE(cache.HasProtectedCache());

    const auto is_closed = cache.Close(CacheType::kProtected);

    ASSERT_TRUE(is_closed);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());

    const auto result = cache.Open(CacheType::kProtected);

    ASSERT_EQ(result, cache::DefaultCache::Success);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_TRUE(cache.HasProtectedCache());
  }

  {
    SCOPED_TRACE("Open/Close invalid cache");

    DefaultCacheImplHelper cache(settings);
    const auto is_closed = cache.Close(CacheType::kProtected);

    ASSERT_FALSE(is_closed);

    const auto result = cache.Open(CacheType::kProtected);

    ASSERT_EQ(result, cache::DefaultCache::NotReady);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());
  }

  {
    SCOPED_TRACE("Close twice");

    DefaultCacheImplHelper cache(settings);
    cache.Open();

    ASSERT_TRUE(cache.HasLruCache());
    ASSERT_TRUE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());

    auto is_closed = cache.Close(CacheType::kMutable);

    ASSERT_TRUE(is_closed);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());

    is_closed = cache.Close(CacheType::kMutable);

    ASSERT_TRUE(is_closed);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());
  }

  {
    SCOPED_TRACE("Open twice");

    DefaultCacheImplHelper cache(settings);
    cache.Open();

    const auto result = cache.Open(CacheType::kMutable);

    ASSERT_EQ(result, cache::DefaultCache::Success);
    ASSERT_TRUE(cache.HasLruCache());
    ASSERT_TRUE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());
  }

  {
    SCOPED_TRACE("Empty cache path, mutable");

    DefaultCacheImplHelper cache({});
    cache.Open();

    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());

    const auto result = cache.Open(CacheType::kMutable);

    ASSERT_EQ(result, cache::DefaultCache::OpenDiskPathFailure);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());
  }

  {
    SCOPED_TRACE("Empty cache path, protected");

    DefaultCacheImplHelper cache({});
    cache.Open();

    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());

    const auto result = cache.Open(CacheType::kProtected);

    ASSERT_EQ(result, cache::DefaultCache::OpenDiskPathFailure);
    ASSERT_FALSE(cache.HasLruCache());
    ASSERT_FALSE(cache.HasMutableCache());
    ASSERT_FALSE(cache.HasProtectedCache());
  }
}

TEST_F(DefaultCacheImplTest, SetMaxMutableSize) {
  cache::CacheSettings settings;
  settings.max_disk_storage = 2000;
  settings.disk_path_mutable = cache_path_;
  DefaultCacheImplHelper cache(settings);
  cache.SetEvictionPortion(85);
  cache.Open();
  cache.Clear();

  std::vector<uint64_t> sizes;
  uint64_t total_size = 0;

  {
    SCOPED_TRACE("Fill the cache with data");

    for (auto i = 0; i < 15; i++) {
      const auto key = std::string("key_") + std::to_string(i);
      auto binary_data = std::vector<unsigned char>(i, i);

      // Create some elements with expiration, and some without
      time_t expiry = 1;
      if (i < 10) {
        expiry = std::numeric_limits<time_t>::max();
      }

      cache.Put(key, std::make_shared<std::vector<unsigned char>>(binary_data),
                expiry);

      const auto size =
          key.size() + binary_data.size() + cache.CalculateExpirySize(key);
      sizes.push_back(size);
      total_size += size;
    }

    ASSERT_EQ(cache.Size(CacheType::kMutable), total_size);
  }

  {
    SCOPED_TRACE("Decrease without eviction");

    EXPECT_EQ(cache.Size(1000), 0);
    EXPECT_EQ(cache.Size(CacheType::kMutable), total_size);
  }

  {
    SCOPED_TRACE("Decrease with eviction");

    // Wait for data expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Eviction process:
    // - elements [10; 14] must be evicted due to expiration;
    // - elements [0; 6] must be evicted due to LRU policy.
    // Elements [7; 9] must stay in the cache
    uint64_t left_size = 0;
    for (auto i = 7; i <= 9; i++) {
      left_size += sizes[i];
    }

    const auto new_max_size = 50;
    const auto max_disk_used_threshold = 0.85;
    EXPECT_EQ(cache.Size(new_max_size), total_size - left_size);
    EXPECT_EQ(cache.Size(CacheType::kMutable), left_size);
    EXPECT_LE(cache.Size(CacheType::kMutable),
              new_max_size * max_disk_used_threshold);
  }

  {
    SCOPED_TRACE("Increase cache size");

    total_size = cache.Size(CacheType::kMutable);
    EXPECT_EQ(cache.Size(1000), 0);
    EXPECT_EQ(cache.Size(CacheType::kMutable), total_size);

    const auto key = std::string("new_key");
    auto binary_data = std::vector<unsigned char>(500, 'a');
    cache.Put(key, std::make_shared<std::vector<unsigned char>>(binary_data),
              1);
    const auto new_item_size =
        key.size() + binary_data.size() + cache.CalculateExpirySize(key);

    EXPECT_EQ(cache.Size(CacheType::kMutable), total_size + new_item_size);
  }
}

TEST_F(DefaultCacheImplTest, ProtectedCacheSize) {
  cache::CacheSettings settings;
  settings.max_disk_storage = std::uint64_t(-1);
  settings.max_memory_cache_size = 0;
  settings.openOptions = olp::cache::OpenOptions::Default;
  settings.eviction_policy = olp::cache::EvictionPolicy::kNone;

  {
    // Filling up cache with arbitrary data
    settings.disk_path_mutable = cache_path_;

    DefaultCacheImplHelper cache{settings};
    ASSERT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);

    boost::uuids::random_generator gen;
    for (int i = 0; i < 100; ++i) {
      auto data_str = boost::uuids::to_string(gen());
      auto data = std::make_shared<cache::KeyValueCache::ValueType>(
          data_str.begin(), data_str.end());
      ASSERT_TRUE(cache.Put(boost::uuids::to_string(gen()), data,
                            std::time(nullptr) * 2));
    }

    for (int i = 0; i < 3; ++i) {
      cache.Compact();
    }

    settings.disk_path_mutable = boost::none;
  }

  const uint64_t actual_size_on_disk = GetCacheSizeOnDisk();
  ASSERT_NE(actual_size_on_disk, 0);

  settings.disk_path_protected = cache_path_;

  DefaultCacheImplHelper cache{settings};
  ASSERT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);

  const auto evaluated_size = cache.Size(CacheType::kProtected);

  const int64_t diff = static_cast<int64_t>(actual_size_on_disk) -
                       static_cast<int64_t>(evaluated_size);
  const double average =
      static_cast<double>(actual_size_on_disk + evaluated_size) / 2.0;
  const double diff_percentage = (diff / average) * 100 /* % */;
  const double acceptable_diff_percentage = 1.2 /* % */;

  EXPECT_LT(std::fabs(diff_percentage), acceptable_diff_percentage);
}

struct OpenTestParameters {
  olp::cache::DefaultCache::StorageOpenResult expected_result;
  olp::cache::OpenOptions open_options;
  boost::optional<std::string> disk_path_mutable;
  boost::optional<std::string> disk_path_protected;
};

class DefaultCacheImplOpenTest
    : public DefaultCacheImplTest,
      public testing::WithParamInterface<OpenTestParameters> {};

TEST_P(DefaultCacheImplOpenTest, ReadOnlyDir) {
  const auto setup_dir = [&](const boost::optional<std::string>& cache_path) {
    if (cache_path) {
      if (olp::utils::Dir::Exists(*cache_path)) {
        ASSERT_TRUE(olp::utils::Dir::Remove(*cache_path));
      }
      ASSERT_TRUE(olp::utils::Dir::Create(*cache_path));
      ASSERT_TRUE(SetRights(*cache_path, true));
    }
  };

  const auto reset_dir = [&](const boost::optional<std::string>& cache_path) {
    if (cache_path) {
      ASSERT_TRUE(olp::utils::Dir::Remove(*cache_path));
    }
  };

  const OpenTestParameters test_params = GetParam();

  ASSERT_NO_FATAL_FAILURE(setup_dir(test_params.disk_path_mutable));
  ASSERT_NO_FATAL_FAILURE(setup_dir(test_params.disk_path_protected));

  cache::CacheSettings settings;
  settings.disk_path_mutable = test_params.disk_path_mutable;
  settings.disk_path_protected = test_params.disk_path_protected;
  settings.openOptions = test_params.open_options;
  DefaultCacheImplHelper cache(settings);
  EXPECT_EQ(test_params.expected_result, cache.Open());

  ASSERT_NO_FATAL_FAILURE(reset_dir(test_params.disk_path_mutable));
  ASSERT_NO_FATAL_FAILURE(reset_dir(test_params.disk_path_protected));
}

std::vector<OpenTestParameters> DefaultCacheImplOpenParams() {
  const std::string cache_path =
      olp::utils::Dir::TempDirectory() + "/unittest_readonly";
  return {{olp::cache::DefaultCache::StorageOpenResult::Success,
           olp::cache::OpenOptions::Default, boost::none, cache_path},
          {olp::cache::DefaultCache::StorageOpenResult::Success,
           olp::cache::OpenOptions::ReadOnly, boost::none, cache_path},
          {olp::cache::DefaultCache::StorageOpenResult::OpenDiskPathFailure,
           olp::cache::OpenOptions::Default, cache_path, boost::none},
          {olp::cache::DefaultCache::StorageOpenResult::OpenDiskPathFailure,
           olp::cache::OpenOptions::ReadOnly, cache_path, boost::none}};
}

INSTANTIATE_TEST_SUITE_P(, DefaultCacheImplOpenTest,
                         testing::ValuesIn(DefaultCacheImplOpenParams()));

}  // namespace
