/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "olp/core/cache/DefaultCache.h"

#include <memory>
#include <string>
#include <utility>

#include "DiskCacheLmDb.h"
#include "InMemoryCache.h"
#include "ProtectedKeyList.h"

namespace olp {
namespace cache {

class DefaultCacheImpl {
 public:
  explicit DefaultCacheImpl(CacheSettings settings);

  ~DefaultCacheImpl();

  DefaultCache::StorageOpenResult Open();
  DefaultCache::StorageOpenResult Open(DefaultCache::CacheType type);

  void Close();
  bool Close(DefaultCache::CacheType type);

  bool Clear();

  void Compact();

  bool Put(const std::string& key, const KeyValueCache::ValueTypePtr value,
           time_t expiry);

  bool Put(const std::string& key, const boost::any& value,
           const Encoder& encoder, time_t expiry);

  boost::any Get(const std::string& key, const Decoder& decoder);

  DefaultCache::ValueTypePtr Get(const std::string& key);

  bool Remove(const std::string& key);

  bool RemoveKeysWithPrefix(const std::string& key);

  bool Contains(const std::string& key) const;
  bool Protect(const DefaultCache::KeyListType& keys);
  bool Release(const DefaultCache::KeyListType& keys);
  bool IsProtected(const std::string& key) const;

  uint64_t Size(DefaultCache::CacheType type) const;
  uint64_t Size(uint64_t new_size);

 protected:
  /// The LRU value property.
  struct ValueProperties {
    size_t size{0ull};
    time_t expiry{KeyValueCache::kDefaultExpiry};
  };

  /// The LRU cache definition using the leveldb keys as key and the value size
  /// as value.
  using DiskLruCache = utils::LruCache<std::string, ValueProperties>;

  /// Returns LRU mutable cache, used for tests.
  const std::unique_ptr<DiskLruCache>& GetMutableCacheLru() const {
    return mutable_cache_lru_;
  }

  /// Returns mutable or protected cache, used for tests.
  const std::unique_ptr<DiskCacheLmDb>& GetCache(
      DefaultCache::CacheType type) const {
    if (type == DefaultCache::CacheType::kMutable) {
      return mutable_cache_;
    }

    return protected_cache_;
  }

  /// Returns memory cache, used for tests.
  const std::unique_ptr<InMemoryCache>& GetMemoryCache() const {
    return memory_cache_;
  }

  /// Gets expiry key, used for tests.
  std::string GetExpiryKey(const std::string& key) const;

  /// Sets eviction portion, used for tests.
  void SetEvictionPortion(uint64_t size);

 private:
  /// Represents intermediate eviction result.
  struct EvictionResult {
    /// Number of evicted elements.
    unsigned count;
    /// The size of evicted elements.
    uint64_t size;
  };

  // Add single key to LRU. (lmdb specific method)
  bool AddKeyLru(std::string key, const MDB_val& value);

  /// Initializes LRU mutable cache if possible.
  void InitializeLru();

  /// Removes key from the mutable lru cache;
  bool RemoveKeyLru(const std::string& key);

  /// Removes all keys with specified prefix from LRU mutable cache.
  void RemoveKeysWithPrefixLru(const std::string& key);

  /// Returns true if key is found in the LRU or protected cache, false -
  /// otherwise.
  bool PromoteKeyLru(const std::string& key);

  /// Returns evicted data size.
  uint64_t MaybeEvictData();

  /// Returns number of evicted elements, evicted data size and a flag indicatin
  /// if eviction limit reached. If the flag is true, another
  /// EvictExpiredDataPortion call is needed to continue eviction.
  EvictionResult EvictExpiredDataPortion(CursorWrapper* const cursor,
                                         uint64_t target_eviction_size);

  /// Returns number of evicted elements, evicted data size and a flag indicatin
  /// if eviction limit reached. If the flag is true, another EvictDataPortion
  /// call is needed to continue eviction.
  /// To be used when all expired data already evicted and it is not enough.
  EvictionResult EvictDataPortion(CursorWrapper* const cursor,
                                  uint64_t target_eviction_size);

  /// Returns changed data size.
  int64_t MaybeUpdatedProtectedKeys(CursorWrapper* const cursor);

  /// Puts data into the mutable cache
  bool PutMutableCache(const std::string& key, const std::string& value,
                       time_t expiry);

  DefaultCache::StorageOpenResult SetupStorage();

  DefaultCache::StorageOpenResult SetupProtectedCache();

  DefaultCache::StorageOpenResult SetupMutableCache();

  void DestroyCache(DefaultCache::CacheType type);

  bool GetFromDiskCache(const std::string& key,
                        KeyValueCache::ValueTypePtr& value, time_t& expiry);

  boost::optional<std::pair<std::string, time_t>> GetFromDiscCache(
      const std::string& key);

  time_t GetExpiryForMemoryCache(const std::string& key,
                                 const time_t& expiry) const;

  CacheSettings settings_;
  bool is_open_;
  std::unique_ptr<InMemoryCache> memory_cache_;
  std::unique_ptr<DiskCacheLmDb> mutable_cache_;
  std::unique_ptr<DiskLruCache> mutable_cache_lru_;
  std::unique_ptr<DiskCacheLmDb> protected_cache_;
  uint64_t mutable_cache_data_size_;
  ProtectedKeyList protected_keys_;
  mutable std::mutex cache_lock_;
  uint64_t eviction_portion_;
};

}  // namespace cache
}  // namespace olp
