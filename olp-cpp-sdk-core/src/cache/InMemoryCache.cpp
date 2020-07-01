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

#include "InMemoryCache.h"

namespace olp {
namespace cache {
namespace {
inline bool HasExpiry(time_t expiry_seconds) {
  return (expiry_seconds != InMemoryCache::kExpiryMax);
}
}  // namespace

InMemoryCache::InMemoryCache(size_t max_size, ModelCacheCostFunc cache_cost,
                             TimeProvider time_provider)
    : item_tuples_(max_size, std::move(cache_cost)),
      time_provider_(std::move(time_provider)) {
  item_tuples_.SetEvictionCallback(
      [this](const std::string& key, ItemTuple&& value) {
        OnEviction(key, std::move(value));
      });
}

bool InMemoryCache::Put(const std::string& key, const boost::any& item,
                        time_t expire_seconds, size_t size) {
  std::lock_guard<std::mutex> lock{mutex_};

  PurgeExpired();

  bool expires = HasExpiry(expire_seconds);
  if (expires) {
    // can't expire in the past.
    if (expire_seconds <= 0) {
      return false;
    }
    expire_seconds += time_provider_();
  }

  auto item_tuple = std::make_tuple(key, expire_seconds, item, size);
  auto ret = item_tuples_.InsertOrAssign(key, item_tuple);
  if (ret.second && expires) {
    item_expiries_[expire_seconds].push_back(item_tuple);
  }

  return ret.second;
}

boost::any InMemoryCache::Get(const std::string& key) {
  std::lock_guard<std::mutex> lock{mutex_};
  auto it = item_tuples_.Find(key);
  if (it != item_tuples_.end()) {
    auto expiry_time = std::get<1>(it.value());
    if (expiry_time < time_provider_()) {
      PurgeExpired(expiry_time);
      return {};
    }

    return std::get<2>(it.value());
  }

  return {};
}

size_t InMemoryCache::Size() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return item_tuples_.Size();
}

void InMemoryCache::Clear() {
  std::lock_guard<std::mutex> lock{mutex_};
  item_expiries_.clear();
  item_tuples_.Clear();
}

bool InMemoryCache::Remove(const std::string& key) {
  std::lock_guard<std::mutex> lock{mutex_};
  return item_tuples_.Erase(key);
}

void InMemoryCache::RemoveKeysWithPrefix(const std::string& key_prefix) {
  std::lock_guard<std::mutex> lock{mutex_};
  for (auto it = item_tuples_.begin(); it != item_tuples_.end();) {
    if (it->key().substr(0, key_prefix.length()) == key_prefix) {
      // we allow concurrent modifications.
      it = item_tuples_.Erase(it);
    } else {
      ++it;
    }
  }
}

bool InMemoryCache::Contains(const std::string& key) {
  std::lock_guard<std::mutex> lock{mutex_};
  auto it = item_tuples_.Find(key);
  if (it != item_tuples_.end()) {
    auto expiry_time = std::get<1>(it.value());
    if (expiry_time < time_provider_()) {
      PurgeExpired(expiry_time);
      return false;
    }

    return true;
  }

  return false;
}

bool InMemoryCache::PurgeExpired() {
  bool ret = true;
  std::vector<time_t> expired_keys;
  const auto time_now = time_provider_();

  for (const auto& item : item_expiries_) {
    if (item.first < time_now) {
      expired_keys.push_back(item.first);
    } else {
      // std::map is sorted, so any further expiry times will be larger.
      break;
    }
  }

  for (auto& key : expired_keys) {
    ret &= PurgeExpired(key);
  }

  return ret;
}

bool InMemoryCache::PurgeExpired(time_t expire_time) {
  bool ret = true;
  for (auto& item : item_expiries_[expire_time]) {
    ret &= item_tuples_.Erase(std::get<0>(item));
  }

  item_expiries_.erase(expire_time);
  return ret;
}

void InMemoryCache::OnEviction(const std::string& key, ItemTuple&& value) {
  time_t expiry = std::get<1>(value);
  if (HasExpiry(expiry)) {
    auto item = item_expiries_[expiry];
    for (auto it = item.begin(); it != item.end(); it++) {
      if (std::get<0>(*it) == key) {
        item.erase(it);
        break;
      }
    }

    if (item_expiries_[expiry].size() == 0) {
      item_expiries_.erase(expiry);
    }
  }
}

}  // namespace cache
}  // namespace olp
