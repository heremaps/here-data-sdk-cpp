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

#include <cache/DefaultCacheImpl.h>
#include <olp/core/utils/Dir.h>

using namespace olp::cache;

namespace {
class DefaultCacheImplHelper : public DefaultCacheImpl {
 public:
  DefaultCacheImplHelper(const CacheSettings& settings)
      : DefaultCacheImpl(settings){};

  bool HasLruCache() const { return GetMutableCacheLru().get() != nullptr; }

  bool ContainsLru(const std::string& key) const {
    const auto& lru_cache = GetMutableCacheLru();
    if (!lru_cache) {
      return false;
    }

    return lru_cache->FindNoPromote(key) != lru_cache->end();
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

TEST(DefaultCacheImplTest, LruCache) {
  std::string cache_path = olp::utils::Dir::TempDirectory() + "/unittest";

  {
    SCOPED_TRACE("Successful creation");

    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_TRUE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("kLeastRecentlyUsed eviction policy");

    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path;
    settings.eviction_policy = EvictionPolicy::kLeastRecentlyUsed;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_TRUE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("Close");

    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path;
    DefaultCacheImplHelper cache(settings);
    cache.Open();
    cache.Close();

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("No Open() call");

    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path;
    DefaultCacheImplHelper cache(settings);

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("Default settings");

    olp::cache::CacheSettings settings;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("No disk cache size limit");

    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path;
    settings.max_disk_storage = std::uint64_t(-1);
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_FALSE(cache.HasLruCache());
  }

  {
    SCOPED_TRACE("kNone eviction policy");

    olp::cache::CacheSettings settings;
    settings.disk_path_mutable = cache_path;
    settings.eviction_policy = EvictionPolicy::kNone;
    DefaultCacheImplHelper cache(settings);
    cache.Open();

    EXPECT_FALSE(cache.HasLruCache());
  }
}

TEST(DefaultCacheImplTest, LruCachePut) {
  olp::cache::CacheSettings settings;
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

TEST(DefaultCacheImplTest, LruCacheGetPromote) {
  olp::cache::CacheSettings settings;
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

    auto value =
        cache.Get(key1, [=](const std::string&) { return data_string; });
    auto it = cache.BeginLru();

    EXPECT_EQ(key1, it->key());
  }

  {
    SCOPED_TRACE("Get binary promote");

    auto value = cache.Get(key2);
    auto it = cache.BeginLru();

    EXPECT_EQ(key2, it->key());
  }
}

TEST(DefaultCacheImplTest, LruCacheRemove) {
  olp::cache::CacheSettings settings;
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

}  // namespace