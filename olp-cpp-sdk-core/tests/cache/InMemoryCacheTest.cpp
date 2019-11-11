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
std::string Key(int index) { return "key" + std::to_string(index); }

std::string Value(int index) { return "value" + std::to_string(index); }

void Populate(olp::cache::InMemoryCache& cache, int count, int first = 0) {
  for (int i = 0; i < count; i++) {
    cache.Put(Key(i + first), Value(i + first));
  }
}

using ItemTuple = olp::cache::InMemoryCache::ItemTuple;
using EqualityCacheCost = olp::utils::CacheCost<ItemTuple>;
using Data = std::shared_ptr<std::vector<unsigned char>>;

Data CreateDataContainer(int length) {
  std::vector<unsigned char> data;
  for (auto i = 0; i < length; i++) {
    data.push_back(i);
  }

  return std::make_shared<std::vector<unsigned char>>(data);
}

TEST(InMemoryCacheTest, Empty) {
  olp::cache::InMemoryCache cache;
  ASSERT_TRUE(cache.Get("keyNotExist").empty());
  ASSERT_EQ(0u, cache.Size());
}

TEST(InMemoryCacheTest, NoLimit) {
  olp::cache::InMemoryCache cache;

  Populate(cache, 10);
  ASSERT_EQ(10u, cache.Size());

  auto i0 = cache.Get("key0");
  ASSERT_FALSE(i0.empty());
  ASSERT_EQ("value0", boost::any_cast<std::string>(i0));

  auto i9 = cache.Get("key9");
  ASSERT_FALSE(i9.empty());
  ASSERT_EQ("value9", boost::any_cast<std::string>(i9));

  ASSERT_TRUE(cache.Get("key10").empty());
}

TEST(InMemoryCacheTest, PutTooLarge) {
  olp::cache::InMemoryCache cache(0);
  std::string oversized("oversized");

  ASSERT_FALSE(cache.Put(oversized, "value: " + oversized));
  ASSERT_TRUE(cache.Get(oversized).empty());
  ASSERT_EQ(0u, cache.Size());
}

TEST(InMemoryCacheTest, Clear) {
  olp::cache::InMemoryCache cache;
  Populate(cache, 10);
  ASSERT_EQ(10u, cache.Size());

  cache.Clear();
  ASSERT_EQ(0u, cache.Size());
}

TEST(InMemoryCacheTest, Remove) {
  olp::cache::InMemoryCache cache;
  Populate(cache, 10);
  ASSERT_EQ(10u, cache.Size());

  cache.Remove(Key(1));
  ASSERT_EQ(9u, cache.Size());
}

TEST(InMemoryCacheTest, RemoveWithPrefix) {
  olp::cache::InMemoryCache cache;
  Populate(cache, 11);
  ASSERT_EQ(11u, cache.Size());

  cache.RemoveKeysWithPrefix(Key(1));  // removes "key1", "key10"
  ASSERT_EQ(9u, cache.Size());         // "key2" .. "key9"

  cache.RemoveKeysWithPrefix(Key(4));
  ASSERT_EQ(8u, cache.Size());  // "key2" .. "key3", "key5" .. "key9"

  cache.RemoveKeysWithPrefix("doesnotexist");
  ASSERT_EQ(8u, cache.Size());  // "key2" .. "key3", "key5" .. "key9"

  cache.RemoveKeysWithPrefix("key");
  ASSERT_EQ(0u, cache.Size());
}

TEST(InMemoryCacheTest, PutOverwritesPrevious) {
  olp::cache::InMemoryCache cache;

  std::string key("duplicateKey");
  std::string orig_value("original");

  cache.Put(key, orig_value);

  ASSERT_EQ(1u, cache.Size());
  auto value = cache.Get(key);
  ASSERT_FALSE(value.empty());
  ASSERT_EQ(orig_value, boost::any_cast<std::string>(value));

  std::string updated_value("updatedValue");
  cache.Put(key, updated_value);

  value = cache.Get(key);
  ASSERT_FALSE(value.empty());
  ASSERT_EQ(updated_value, boost::any_cast<std::string>(value));
}

TEST(InMemoryCacheTest, InsertOverLimit) {
  olp::cache::InMemoryCache cache(1);
  Populate(cache, 2);

  ASSERT_EQ(1u, cache.Size());
  ASSERT_TRUE(cache.Get("key0").empty());
  auto i1 = cache.Get("key1");
  ASSERT_FALSE(i1.empty());
  ASSERT_EQ("value1", boost::any_cast<std::string>(i1));
}

TEST(InMemoryCacheTest, GetReorders) {
  olp::cache::InMemoryCache cache(2);
  Populate(cache, 2);
  ASSERT_EQ(2u, cache.Size());

  auto i0 = cache.Get("key0");
  ASSERT_FALSE(i0.empty());
  ASSERT_EQ("value0", boost::any_cast<std::string>(i0));

  Populate(cache, 1, 2);
  ASSERT_TRUE(cache.Get("key1").empty());

  auto i2 = cache.Get("key2");
  ASSERT_FALSE(i2.empty());
  ASSERT_EQ("value2", boost::any_cast<std::string>(i2));
}

TEST(InMemoryCacheTest, GetSingleExpired) {
  time_t now = std::time(nullptr);

  olp::cache::InMemoryCache cache(10, EqualityCacheCost(), [&] { return now; });

  std::string with_expiry("withExpiry");
  std::string with_later_expiry("withLaterExpiry");
  std::string no_expiry("noExpiry");

  cache.Put(with_expiry, "value: " + with_expiry, 1);
  cache.Put(with_later_expiry, "value: " + with_later_expiry, 10);
  cache.Put(no_expiry, "value: " + no_expiry);
  ASSERT_EQ(3u, cache.Size());

  // wait 2 seconds
  now += 2;

  // cache doesn't purge expired until we call 'Get' or 'Put'
  ASSERT_EQ(3u, cache.Size());

  // withExpiry should be expired, withLaterExpiry and noExpiry still value
  ASSERT_TRUE(cache.Get(with_expiry).empty());

  auto with_later_expiry_val = cache.Get(with_later_expiry);
  ASSERT_FALSE(with_later_expiry_val.empty());
  auto no_expiry_val = cache.Get(no_expiry);
  ASSERT_FALSE(no_expiry_val.empty());

  ASSERT_EQ(2u, cache.Size());
}

TEST(InMemoryCacheTest, GetMultipleExpired) {
  time_t now = std::time(nullptr);

  olp::cache::InMemoryCache cache(10, EqualityCacheCost(), [&] { return now; });

  std::string with_expiry("withExpiry");
  std::string with_same_expiry("withSameExpiry");
  std::string with_later_expiry("withLaterExpiry");
  std::string no_expiry("noExpiry");

  cache.Put(with_expiry, "value: " + with_expiry, 1);
  cache.Put(with_same_expiry, "value: " + with_same_expiry, 1);
  cache.Put(with_later_expiry, "value: " + with_later_expiry, 10);
  cache.Put(no_expiry, "value: " + no_expiry);
  ASSERT_EQ(4u, cache.Size());

  // wait 2 seconds
  now += 2;

  // cache doesn't purge expired until we call 'Get' or 'Put'
  ASSERT_EQ(4u, cache.Size());

  // with_expiry and with_same_expiry should be expired,
  // with_later_expiry and no_expiry still valid
  ASSERT_TRUE(cache.Get(with_expiry).empty());
  ASSERT_TRUE(cache.Get(with_same_expiry).empty());
  ASSERT_FALSE(cache.Get(with_later_expiry).empty());
  ASSERT_FALSE(cache.Get(no_expiry).empty());

  ASSERT_EQ(2u, cache.Size());
}

TEST(InMemoryCacheTest, PutMultipleExpired) {
  time_t now = std::time(nullptr);

  olp::cache::InMemoryCache cache(10, EqualityCacheCost(), [&] { return now; });

  std::string with_expiry("withExpiry");
  std::string with_same_expiry("withSameExpiry");
  std::string with_later_expiry("withLaterExpiry");
  std::string no_expiry("noExpiry");

  cache.Put(with_expiry, "value: " + with_expiry, 1);
  cache.Put(with_same_expiry, "value: " + with_same_expiry, 1);
  cache.Put(with_later_expiry, "value: " + with_later_expiry, 10);
  cache.Put(no_expiry, "value: " + no_expiry);
  ASSERT_EQ(4u, cache.Size());

  // wait 2 seconds
  now += 2;

  // cache doesn't purge expired until we call 'Get' or 'Put'

  std::string trigger("trigger");
  cache.Put(trigger, "value: " + trigger);

  // with_expiry and with_same_expiry should be expired,
  // with_later_expiry and no_expiry still valid
  ASSERT_EQ(3u, cache.Size());
  ASSERT_TRUE(cache.Get(with_expiry).empty());
  ASSERT_TRUE(cache.Get(with_same_expiry).empty());
  ASSERT_FALSE(cache.Get(with_later_expiry).empty());
  ASSERT_FALSE(cache.Get(no_expiry).empty());
  ASSERT_FALSE(cache.Get(trigger).empty());
}

TEST(InMemoryCacheTest, ItemWithExpiryEvicted) {
  time_t now = std::time(nullptr);

  // max 2
  olp::cache::InMemoryCache cache(2, EqualityCacheCost(), [&] { return now; });

  // insert with expiry
  std::string with_expiry("withExpiry");
  cache.Put(with_expiry, "value: " + with_expiry, 1);

  // insert 2 more
  std::string no_expiry("noExpiry");
  std::string another("another");

  cache.Put(no_expiry, "value: " + no_expiry);
  cache.Put(another, "value: " + another);

  ASSERT_EQ(2u, cache.Size());

  // wait 2 seconds
  now += 2;

  // Get items
  ASSERT_FALSE(cache.Get(no_expiry).empty());
  ASSERT_FALSE(cache.Get(another).empty());
}

TEST(InMemoryCacheTest, ItemsWithExpiryEvicted) {
  time_t now = std::time(nullptr);

  // max 2
  olp::cache::InMemoryCache cache(2, EqualityCacheCost(), [&] { return now; });

  // insert 2 with same expiry
  std::string with_expiry("withExpiry");
  cache.Put(with_expiry, "value: " + with_expiry, 1);
  std::string dup_expiry("dupExpiry");
  cache.Put(dup_expiry, "value: " + dup_expiry, 1);

  // insert with same expiry
  std::string trip_expiry("tripExpiry");
  cache.Put(trip_expiry, "value: " + trip_expiry, 1);
  ASSERT_TRUE(cache.Get(with_expiry).empty());
  ASSERT_FALSE(cache.Get(dup_expiry).empty());
  ASSERT_FALSE(cache.Get(trip_expiry).empty());

  // insert without expiry
  std::string no_expiry("noExpiry");
  cache.Put(no_expiry, "value: " + no_expiry);
  ASSERT_TRUE(cache.Get(dup_expiry).empty());
  ASSERT_FALSE(cache.Get(trip_expiry).empty());
  ASSERT_FALSE(cache.Get(no_expiry).empty());

  // insert with same expiry
  std::string same_expiry("sameExpiry");
  cache.Put(same_expiry, "value: " + same_expiry, 1);

  // wait 2 seconds
  now += 2;

  // Get remaining item
  ASSERT_TRUE(cache.Get(with_expiry).empty());
  ASSERT_TRUE(cache.Get(dup_expiry).empty());
  ASSERT_TRUE(cache.Get(trip_expiry).empty());
  ASSERT_TRUE(cache.Get(same_expiry).empty());
  ASSERT_FALSE(cache.Get(no_expiry).empty());
}

TEST(InMemoryCacheTest, CustomCost) {
  std::string oversized("oversized");
  auto oversized_model = "value: " + oversized;

  struct MyCacheCost {
    std::size_t operator()(const ItemTuple& s) const { return 2; }
  };

  auto cost_func = [](const ItemTuple& tuple) { return 2; };

  olp::cache::InMemoryCache cache(1, MyCacheCost());

  ASSERT_FALSE(cache.Put(oversized, oversized_model));
  ASSERT_TRUE(cache.Get(oversized).empty());
  ASSERT_EQ(0u, cache.Size());

  olp::cache::InMemoryCache cache2(1, std::move(cost_func));

  ASSERT_FALSE(cache2.Put(oversized, oversized_model));
  ASSERT_TRUE(cache2.Get(oversized).empty());
  ASSERT_EQ(0u, cache2.Size());
}

TEST(InMemoryCacheTest, StaticNoLimit) {
  olp::cache::InMemoryCache cache;

  Populate(cache, 10);
  ASSERT_EQ(10u, cache.Size());

  olp::cache::InMemoryCache cache2;

  Populate(cache2, 10);
  ASSERT_EQ(10u, cache2.Size());
}

TEST(InMemoryCacheTest, ClassBasedCustomCost) {
  auto class_model_cache_cost = [](const ItemTuple& tuple) {
    auto model_item = std::get<2>(tuple);
    std::size_t result(1u);

    if (auto data_container = boost::any_cast<Data>(model_item)) {
      auto data_size = data_container->size();
      result = (data_size > 0) ? data_size : result;
    }

    return result;
  };

  olp::cache::InMemoryCache cache(10, class_model_cache_cost);

  std::string empty("empty");
  auto empty_container = CreateDataContainer(0);

  {
    ASSERT_TRUE(cache.Put(empty, empty_container));
    ASSERT_FALSE(cache.Get(empty).empty());
    ASSERT_EQ(1u, cache.Size());
    cache.Clear();
    ASSERT_EQ(0u, cache.Size());
  }

  std::string max("max");
  auto max_container = CreateDataContainer(10);

  {
    ASSERT_TRUE(cache.Put(max, max_container));
    ASSERT_FALSE(cache.Get(max).empty());
    ASSERT_EQ(10u, cache.Size());

    ASSERT_TRUE(cache.Put(empty, empty_container));
    ASSERT_FALSE(cache.Get(empty).empty());
    ASSERT_EQ(1u, cache.Size());

    cache.Clear();
    ASSERT_EQ(0u, cache.Size());
  }

  {
    std::string oversize("oversize");
    auto oversize_container = CreateDataContainer(11);

    ASSERT_FALSE(cache.Put(oversize, oversize_container));
    ASSERT_TRUE(cache.Get(oversize).empty());
    ASSERT_EQ(0u, cache.Size());
  }
}
}  // namespace
