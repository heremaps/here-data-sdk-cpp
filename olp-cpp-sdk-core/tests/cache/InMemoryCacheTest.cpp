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
#include <string>

#include "InMemoryCache.h"

namespace {
  std::string key(int index) {
    return "key" + std::to_string(index);
  }

  std::string value(int index){
    return "value" + std::to_string(index);
  }

  void populate(olp::cache::InMemoryCache& cache, int count, int first) {
    for (int i = 0; i < count; i++) {
        cache.Put(key(i + first), value(i + first));
    }
  }

  void populate(olp::cache::InMemoryCache& cache, int count) {
    populate(cache, count, 0);
  }
};

TEST(InMemoryCacheTest, Empty) {
  olp::cache::InMemoryCache inMemCache;

  ASSERT_TRUE(inMemCache.Get("keyNotExist").empty());
  ASSERT_EQ(0u, inMemCache.Size());
}

TEST(InMemoryCacheTest, NoLimit) {
  olp::cache::InMemoryCache inMemCache;

  populate(inMemCache, 10);
  ASSERT_EQ(10u, inMemCache.Size());

  auto i0 = inMemCache.Get("key0");
  ASSERT_FALSE(i0.empty());
  ASSERT_EQ("value0", boost::any_cast<std::string>(i0));

  auto i9 = inMemCache.Get("key9");
  ASSERT_FALSE(i9.empty());
  ASSERT_EQ("value9", boost::any_cast<std::string>(i9));

  ASSERT_TRUE(inMemCache.Get("key10").empty());
}

TEST(InMemoryCacheTest, PutTooLarge) {
  olp::cache::InMemoryCache inMemCache(0);
  std::string oversized("oversized");

  ASSERT_FALSE(inMemCache.Put(oversized, "value: " + oversized));
  ASSERT_TRUE(inMemCache.Get(oversized).empty());
  ASSERT_EQ(0u, inMemCache.Size());
}

TEST(InMemoryCacheTest, Clear) {
  olp::cache::InMemoryCache inMemCache;
  populate(inMemCache, 10);
  ASSERT_EQ(10u, inMemCache.Size());

  inMemCache.Clear();
  ASSERT_EQ(0u, inMemCache.Size());
}

TEST(InMemoryCacheTest, Remove) {
  olp::cache::InMemoryCache inMemCache;
  populate(inMemCache, 10);
  ASSERT_EQ(10u, inMemCache.Size());

  inMemCache.Remove(key(1));
  ASSERT_EQ(9u, inMemCache.Size());
}

TEST(InMemoryCacheTest, RemoveWithPrefix) {
  olp::cache::InMemoryCache inMemCache;
  populate(inMemCache, 11);
  ASSERT_EQ(11u, inMemCache.Size());

  inMemCache.RemoveKeysWithPrefix(key(1));  // removes "key1", "key10"
  ASSERT_EQ(9u, inMemCache.Size());          // "key2" .. "key9"

  inMemCache.RemoveKeysWithPrefix(key(4));
  ASSERT_EQ(8u, inMemCache.Size());  // "key2" .. "key3", "key5" .. "key9"

  inMemCache.RemoveKeysWithPrefix("doesnotexist");
  ASSERT_EQ(8u, inMemCache.Size());  // "key2" .. "key3", "key5" .. "key9"

  inMemCache.RemoveKeysWithPrefix("key");
  ASSERT_EQ(0u, inMemCache.Size());
}

TEST(InMemoryCacheTest, PutOverwritesPrevious) {
  olp::cache::InMemoryCache inMemCache;

  std::string key("duplicateKey");
  std::string origValue("original");

  inMemCache.Put(key, origValue);

  ASSERT_EQ(1u, inMemCache.Size());
  auto value = inMemCache.Get(key);
  ASSERT_FALSE(value.empty());
  ASSERT_EQ(origValue, boost::any_cast<std::string>(origValue));

  std::string updatedValue("updatedValue");
  inMemCache.Put(key, updatedValue);

  value = inMemCache.Get(key);
  ASSERT_FALSE(value.empty());
  ASSERT_EQ(updatedValue, boost::any_cast<std::string>(value));
}

TEST(InMemoryCacheTest, InsertOverLimit) {
  olp::cache::InMemoryCache inMemCache(1);
  populate(inMemCache, 2);
  ASSERT_EQ(1u, inMemCache.Size());
  ASSERT_TRUE(inMemCache.Get("key0").empty());
  auto i1 = inMemCache.Get("key1");
  ASSERT_FALSE(i1.empty());
  ASSERT_EQ("value1", boost::any_cast<std::string>(i1));
}

TEST(InMemoryCacheTest, GetReorders) {
  olp::cache::InMemoryCache inMemCache(2);
  populate(inMemCache, 2);
  ASSERT_EQ(2u, inMemCache.Size());

  auto i0 = inMemCache.Get("key0");
  ASSERT_FALSE(i0.empty());
  ASSERT_EQ("value0", boost::any_cast<std::string>(i0));

  populate(inMemCache, 1, 2);
  ASSERT_TRUE(inMemCache.Get("key1").empty());

  auto i2 = inMemCache.Get("key2");
  ASSERT_FALSE(i2.empty());
  ASSERT_EQ("value2", boost::any_cast<std::string>(i2));
}

TEST(InMemoryCacheTest, GetSingleExpired) {
  time_t now = std::time(nullptr);

  olp::cache::InMemoryCache inMemCache(
      10, olp::cache::InMemoryCache::EqualityCacheCost(),
      [&now]() { return now; });

  std::string withExpiry("withExpiry");
  std::string withLaterExpiry("withLaterExpiry");
  std::string noExpiry("noExpiry");

  inMemCache.Put(withExpiry, "value: " + withExpiry, 1);
  inMemCache.Put(withLaterExpiry, "value: " + withLaterExpiry, 10);
  inMemCache.Put(noExpiry, "value: " + noExpiry);
  ASSERT_EQ(3u, inMemCache.Size());

  // wait 2 seconds
  now += 2;

  // cache doesn't purge expired until we call 'Get' or 'Put'
  ASSERT_EQ(3u, inMemCache.Size());

  // withExpiry should be expired, withLaterExpiry and noExpiry still value
  ASSERT_TRUE(inMemCache.Get(withExpiry).empty());

  auto withLaterExpiryVal
      = inMemCache.Get(withLaterExpiry);
  ASSERT_FALSE(withLaterExpiryVal.empty());
  auto noExpiryVal
      = inMemCache.Get(noExpiry);
  ASSERT_FALSE(noExpiryVal.empty());

  ASSERT_EQ(2u, inMemCache.Size());
}

TEST(InMemoryCacheTest, GetMultipleExpired) {
  time_t now = std::time(nullptr);

  olp::cache::InMemoryCache inMemCache(
      10, olp::cache::InMemoryCache::EqualityCacheCost(),
      [&now]() { return now; });

  std::string withExpiry("withExpiry");
  std::string withSameExpiry("withSameExpiry");
  std::string withLaterExpiry("withLaterExpiry");
  std::string noExpiry("noExpiry");

  inMemCache.Put(withExpiry, "value: " + withExpiry, 1);
  inMemCache.Put(withSameExpiry, "value: " + withSameExpiry, 1);
  inMemCache.Put(withLaterExpiry, "value: " + withLaterExpiry, 10);
  inMemCache.Put(noExpiry, "value: " + noExpiry);
  ASSERT_EQ(4u, inMemCache.Size());

  // wait 2 seconds
  now += 2;

  // cache doesn't purge expired until we call 'Get' or 'Put'
  ASSERT_EQ(4u, inMemCache.Size());

  // withExpiry and withSameExpiry should be expired,
  // withLaterExpiry and noExpiry still valid
  ASSERT_TRUE(inMemCache.Get(withExpiry).empty());
  ASSERT_TRUE(inMemCache.Get(withSameExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(withLaterExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(noExpiry).empty());

  ASSERT_EQ(2u, inMemCache.Size());
}

TEST(InMemoryCacheTest, PutMultipleExpired) {
  time_t now = std::time(nullptr);

  olp::cache::InMemoryCache inMemCache(
      10, olp::cache::InMemoryCache::EqualityCacheCost(),
      [&now]() { return now; });

  std::string withExpiry("withExpiry");
  std::string withSameExpiry("withSameExpiry");
  std::string withLaterExpiry("withLaterExpiry");
  std::string noExpiry("noExpiry");

  inMemCache.Put(withExpiry,
                  "value: " + withExpiry, 1);
  inMemCache.Put(withSameExpiry,
                  "value: " + withSameExpiry, 1);
  inMemCache.Put(withLaterExpiry,
                  "value: " + withLaterExpiry, 10);
  inMemCache.Put(noExpiry, "value: " + noExpiry);
  ASSERT_EQ(4u, inMemCache.Size());

  // wait 2 seconds
  now += 2;

  // cache doesn't purge expired until we call 'Get' or 'Put'

  std::string trigger("trigger");
  inMemCache.Put(trigger, "value: " + trigger);

  // withExpiry and withSameExpiry should be expired,
  // withLaterExpiry and noExpiry still valid
  ASSERT_EQ(3u, inMemCache.Size());
  ASSERT_TRUE(inMemCache.Get(withExpiry).empty());
  ASSERT_TRUE(inMemCache.Get(withSameExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(withLaterExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(noExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(trigger).empty());
}

TEST(InMemoryCacheTest, ItemWithExpiryEvicted) {
  time_t now = std::time(nullptr);

  // max 2
  olp::cache::InMemoryCache inMemCache(
      2, olp::cache::InMemoryCache::EqualityCacheCost(),
      [&now]() { return now; });

  // insert with expiry
  std::string withExpiry("withExpiry");
  inMemCache.Put(withExpiry,
                  "value: " + withExpiry, 1);

  // insert 2 more
  std::string noExpiry("noExpiry");
  std::string another("another");

  inMemCache.Put(noExpiry, "value: " + noExpiry);
  inMemCache.Put(another, "value: " + another);

  ASSERT_EQ(2u, inMemCache.Size());

  // wait 2 seconds
  now += 2;

  // Get items
  ASSERT_FALSE(inMemCache.Get(noExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(another).empty());
}

TEST(InMemoryCacheTest, ItemsWithExpiryEvicted) {
  time_t now = std::time(nullptr);

  // max 2
  olp::cache::InMemoryCache inMemCache(
      2, olp::cache::InMemoryCache::EqualityCacheCost(),
      [&now]() { return now; });

  // insert 2 with same expiry
  std::string withExpiry("withExpiry");
  inMemCache.Put(withExpiry,
                  "value: " + withExpiry, 1);
  std::string dupExpiry("dupExpiry");
  inMemCache.Put(dupExpiry, "value: " + dupExpiry,
                  1);

  // insert with same expiry
  std::string tripExpiry("tripExpiry");
  inMemCache.Put(tripExpiry,
                  "value: " + tripExpiry, 1);
  ASSERT_TRUE(inMemCache.Get(withExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(dupExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(tripExpiry).empty());

  // insert without expiry
  std::string noExpiry("noExpiry");
  inMemCache.Put(noExpiry, "value: " + noExpiry);
  ASSERT_TRUE(inMemCache.Get(dupExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(tripExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(noExpiry).empty());

  // insert with same expiry
  std::string sameExpiry("sameExpiry");
  inMemCache.Put(sameExpiry,
                  "value: " + sameExpiry, 1);

  // wait 2 seconds
  now += 2;

  // Get remaining item
  ASSERT_TRUE(inMemCache.Get(withExpiry).empty());
  ASSERT_TRUE(inMemCache.Get(dupExpiry).empty());
  ASSERT_TRUE(inMemCache.Get(tripExpiry).empty());
  ASSERT_TRUE(inMemCache.Get(sameExpiry).empty());
  ASSERT_FALSE(inMemCache.Get(noExpiry).empty());
}

TEST(InMemoryCacheTest, CustomCost) {
  std::string oversized("oversized");
  auto oversizedModel = "value: " + oversized;

  struct MyCacheCost
  {
    std::size_t
    operator()(const olp::cache::InMemoryCache::ItemTuple& s) const {
      return 2;
    }
  };

  olp::cache::InMemoryCache inMemCache(1, MyCacheCost());

  ASSERT_FALSE(inMemCache.Put(oversized, oversizedModel));
  ASSERT_TRUE(inMemCache.Get(oversized).empty());
  ASSERT_EQ(0u, inMemCache.Size());

  auto costFunc = [](const olp::cache::InMemoryCache::ItemTuple& tuple) {
    return 2;
  };

  olp::cache::InMemoryCache inMemCache2(1, std::move(costFunc));

  ASSERT_FALSE(inMemCache2.Put(oversized, oversizedModel));
  ASSERT_TRUE(inMemCache2.Get(oversized).empty());
  ASSERT_EQ(0u, inMemCache2.Size());
}

TEST(InMemoryCacheTest, StaticNoLimit) {
  olp::cache::InMemoryCache inMemCache;

  populate(inMemCache, 10);
  ASSERT_EQ(10u, inMemCache.Size());

  olp::cache::InMemoryCache inMemCache2;

  populate(inMemCache2, 10);
  ASSERT_EQ(10u, inMemCache2.Size());
}

using Data = std::shared_ptr<std::vector<unsigned char>>;

Data createDataContainer(int length) {
  std::vector< unsigned char > data;
  for (auto i = 0; i < length; i++)
      data.push_back(i);

  return std::make_shared<std::vector<unsigned char>>(data);
}

TEST(InMemoryCacheTest, ClassBasedCustomCost) {
  olp::cache::InMemoryCache::ModelCacheCostFunc classModelCacheCost =
    [](const olp::cache::InMemoryCache::ItemTuple& tuple) {
      auto modelItem = std::get<2>(tuple);

      std::size_t result(1);

      if (auto dataContainer = boost::any_cast<Data>(modelItem)) {
        auto dataSize = dataContainer->size();

        if (dataSize > 0)
            result = dataSize;
      }
      return result;
    };

  olp::cache::InMemoryCache inMemCache(10, classModelCacheCost);

  std::string empty("empty");
  auto emptyContainer = createDataContainer(0);

  {
    ASSERT_TRUE(inMemCache.Put(empty, emptyContainer));
    ASSERT_FALSE(inMemCache.Get(empty).empty());
    ASSERT_EQ(1u, inMemCache.Size());
    inMemCache.Clear();
    ASSERT_EQ(0u, inMemCache.Size());
  }

  std::string max("max");
  auto maxContainer = createDataContainer(10);
  {
    ASSERT_TRUE(inMemCache.Put(max, maxContainer));
    ASSERT_FALSE(inMemCache.Get(max).empty());
    ASSERT_EQ(10u, inMemCache.Size());

    ASSERT_TRUE(inMemCache.Put(empty, emptyContainer));
    ASSERT_FALSE(inMemCache.Get(empty).empty());
    ASSERT_EQ(1u, inMemCache.Size());

    inMemCache.Clear();
    ASSERT_EQ(0u, inMemCache.Size());
  }

  {
    std::string oversize("oversize");
    auto oversizeContainer = createDataContainer(11);

    ASSERT_FALSE(inMemCache.Put(oversize, oversizeContainer));
    ASSERT_TRUE(inMemCache.Get(oversize).empty());
    ASSERT_EQ(0u, inMemCache.Size());
  }
}
