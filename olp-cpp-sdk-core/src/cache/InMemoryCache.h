/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <olp/core/porting/any.h>
#include <olp/core/utils/LruCache.h>

namespace olp {
namespace cache {

/**
 * @brief In-memory cache that implements a LRU and a time based eviction
 * policy.
 */
class InMemoryCache {
 public:
  static constexpr size_t kSizeMax = std::numeric_limits<std::size_t>::max();
  static constexpr time_t kExpiryMax = std::numeric_limits<time_t>::max();

  using ItemTuple = std::tuple<std::string, time_t, olp::porting::any, size_t>;
  using ItemTuples = std::vector<ItemTuple>;
  using TimeProvider = std::function<time_t()>;
  using ModelCacheCostFunc = std::function<std::size_t(const ItemTuple&)>;

  /// Will be used to filter out keys to be removed in case they are protected.
  using RemoveFilterFunc = std::function<bool(const std::string&)>;

  /// Default cache cost based on size.
  struct DefaultCacheCost {
    std::size_t operator()(const ItemTuple& value) const {
      auto result = std::get<3>(value);
      return (result == 0) ? 1u : result;
    }
  };

  /// Default time provider using std::chrono::system_clock.
  struct DefaultTimeProvider {
    time_t operator()() const {
      auto now = std::chrono::system_clock::now();
      return std::chrono::system_clock::to_time_t(now);
    }
  };

  InMemoryCache(size_t max_size = kSizeMax,
                ModelCacheCostFunc cache_cost = DefaultCacheCost(),
                TimeProvider time_provider = DefaultTimeProvider());

  bool Put(const std::string& key, const olp::porting::any& item,
           time_t expire_seconds = kExpiryMax, size_t = 1u);

  olp::porting::any Get(const std::string& key);
  size_t Size() const;
  void Clear();

  bool Remove(const std::string& key);
  void RemoveKeysWithPrefix(const std::string& key_prefix,
                            const RemoveFilterFunc& filter = nullptr);
  bool Contains(const std::string& key) const;

 protected:
  bool PurgeExpired();
  bool PurgeExpired(time_t expire_time);
  void OnEviction(const std::string& key, ItemTuple&& value);

 private:
  mutable std::mutex mutex_;
  utils::LruCache<std::string, ItemTuple, ModelCacheCostFunc> item_tuples_;
  std::map<time_t, ItemTuples> item_expiries_;
  TimeProvider time_provider_;
};
}  // namespace cache
}  // namespace olp
