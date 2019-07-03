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

#include <ctime>

#include "InMemoryCache.h"

namespace olp {
namespace cache {
InMemoryCache::TimeProvider& InMemoryCache::DefaultTimeProvider() {
  static InMemoryCache::TimeProvider defaultTimeProvider = []() {
    return std::time(nullptr);
  };
  return defaultTimeProvider;
}

InMemoryCache::ModelCacheCostFunc& InMemoryCache::EqualityCacheCost() {
  static InMemoryCache::ModelCacheCostFunc equalityCacheCost =
      [](const ItemTuple& tuple) { return 1; };
  return equalityCacheCost;
}

InMemoryCache::ModelCacheCostFunc& InMemoryCache::PresetModelSizeCacheCost() {
  static InMemoryCache::ModelCacheCostFunc presetModelSizeCacheCost =
      [](const InMemoryCache::ItemTuple& tuple) {
        auto result = std::get<3>(tuple);
        if (result == 0) result = 1;
        return result;
      };
  return presetModelSizeCacheCost;
}

InMemoryCache::InMemoryCache(size_t maxSize, ModelCacheCostFunc cacheCost,
                             TimeProvider timeProvider)
    : m_itemTuples(maxSize, std::move(cacheCost)),
      m_timeProvider(std::move(timeProvider)) {
  m_itemTuples.SetEvictionCallback(
      [this](const std::string& key, ItemTuple&& value) {
        OnEviction(key, std::move(value));
      });
}

bool HasExpiry(time_t expirySeconds) {
  return (expirySeconds != std::numeric_limits<time_t>::max());
}

bool InMemoryCache::Put(const std::string& key, const boost::any& item,
                        time_t expireSeconds, size_t size) {
  std::lock_guard<std::mutex> lock{m_mutex};

  PurgeExpired();

  bool expires = HasExpiry(expireSeconds);
  if (expires) {
    // can't expire in the past.
    if (expireSeconds <= 0) return false;

    expireSeconds += m_timeProvider();
  }
  auto itemTuple = std::make_tuple(key, expireSeconds, item, size);
  auto ret = m_itemTuples.InsertOrAssign(key, itemTuple);
  if (ret.second && expires) m_itemExpiries[expireSeconds].push_back(itemTuple);

  return ret.second;
}

boost::any InMemoryCache::Get(const std::string& key) {
  std::lock_guard<std::mutex> lock{m_mutex};
  auto it = m_itemTuples.Find(key);
  if (it != m_itemTuples.end()) {
    auto expiryTime = std::get<1>(it.value());
    if (expiryTime < m_timeProvider()) {
      PurgeExpired(expiryTime);
      return boost::any();
    }

    return std::get<2>(it.value());
  }

  return boost::any();
}

size_t InMemoryCache::Size() const { return m_itemTuples.Size(); }

void InMemoryCache::Clear() {
  std::lock_guard<std::mutex> lock{m_mutex};
  m_itemExpiries.clear();
  m_itemTuples.Clear();
}

void InMemoryCache::Remove(const std::string& key) {
  std::lock_guard<std::mutex> lock{m_mutex};
  m_itemTuples.Erase(key);
}

void InMemoryCache::RemoveKeysWithPrefix(const std::string& keyPrefix) {
  std::lock_guard<std::mutex> lock{m_mutex};
  for (auto it = m_itemTuples.begin(); it != m_itemTuples.end();) {
    if (it->key().substr(0, keyPrefix.length()) == keyPrefix) {
      // we allow concurrent modifications.
      it = m_itemTuples.Erase(it);
    } else {
      ++it;
    }
  }
}

bool InMemoryCache::PurgeExpired() {
  bool ret = true;
  std::vector<time_t> expiredKeys;
  for (auto item : m_itemExpiries) {
    if (item.first < m_timeProvider())
      expiredKeys.push_back(item.first);
    else
      break;  // std::map is sorted, so any further expiry times will be larger.
  }

  for (auto key : expiredKeys) ret &= PurgeExpired(key);

  return ret;
}

bool InMemoryCache::PurgeExpired(time_t expireTime) {
  // TODO schedule a task and do this async?
  for (auto itExp : m_itemExpiries[expireTime]) {
    m_itemTuples.Erase(std::get<0>(itExp));
  }
  m_itemExpiries.erase(expireTime);

  return true;
}

void InMemoryCache::OnEviction(const std::string& key, ItemTuple&& value) {
  time_t expiry = std::get<1>(value);
  bool expires = HasExpiry(expiry);
  if (expires) {
    auto expVec = m_itemExpiries[expiry];
    for (auto itTup = expVec.begin(); itTup != expVec.end(); itTup++) {
      if (std::get<0>(*itTup) == key) {
        expVec.erase(itTup);
        break;
      }
    }

    if (m_itemExpiries[expiry].size() == 0) m_itemExpiries.erase(expiry);
  }
}

}  // namespace cache
}  // namespace olp
