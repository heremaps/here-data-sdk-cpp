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

#pragma once

#include <time.h>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <olp/core/utils/LruCache.h>
#include <boost/any.hpp>

namespace olp {
namespace cache {
class InMemoryCache {
 public:
  using ItemTuple = std::tuple<std::string, time_t, boost::any, size_t>;

  using TimeProvider = std::function<time_t()>;
  static TimeProvider& DefaultTimeProvider();

  using ModelCacheCostFunc = std::function<std::size_t(const ItemTuple&)>;
  static ModelCacheCostFunc& EqualityCacheCost();
  static ModelCacheCostFunc& PresetModelSizeCacheCost();

  InMemoryCache(size_t maxSize = (std::numeric_limits<std::size_t>::max)(),
                ModelCacheCostFunc cacheCost = PresetModelSizeCacheCost(),
                TimeProvider timeProvider = DefaultTimeProvider());

  bool Put(const std::string& key, const boost::any& item,
           time_t expireSeconds = (std::numeric_limits<time_t>::max)(),
           size_t = 1);

  boost::any Get(const std::string& key);
  size_t Size() const;
  void Clear();

  void Remove(const std::string& key);
  void RemoveKeysWithPrefix(const std::string& keyPrefix);

 private:
  bool PurgeExpired();
  bool PurgeExpired(time_t expireTime);
  void OnEviction(const std::string& key, ItemTuple&& value);

 private:
  std::mutex m_mutex;
  utils::LruCache<std::string, ItemTuple, ModelCacheCostFunc> m_itemTuples;
  std::map<time_t, std::vector<ItemTuple> > m_itemExpiries;
  TimeProvider m_timeProvider;
};
}  // namespace cache
}  // namespace olp
