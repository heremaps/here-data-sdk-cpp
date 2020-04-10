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

#include "olp/core/cache/DefaultCache.h"

#include "DiskCache.h"
#include "InMemoryCache.h"

namespace olp {
namespace cache {

class DefaultCacheImpl {
 public:
  DefaultCacheImpl(CacheSettings settings);

  DefaultCache::StorageOpenResult Open();

  void Close();

  bool Clear();

  bool Put(const std::string& key, const KeyValueCache::ValueTypePtr value,
           time_t expiry);

  bool Put(const std::string& key, const boost::any& value,
           const Encoder& encoder, time_t expiry);

  boost::any Get(const std::string& key, const Decoder& decoder);

  DefaultCache::ValueTypePtr Get(const std::string& key);

  bool Remove(const std::string& key);

  bool RemoveKeysWithPrefix(const std::string& key);

 protected:
  /// The LRU cache definition using the leveldb keys as key and the value size
  /// as value.
  using DiskLruCache = utils::LruCache<std::string, size_t>;

  /// Returns LRU mutable cache, used for tests.
  const std::unique_ptr<DiskLruCache>& GetMutableCacheLru() const {
    return mutable_cache_lru_;
  };

  /// Returns mutable cache, used for tests.
  const std::unique_ptr<DiskCache>& GetMutableCache() const {
    return mutable_cache_;
  };

  /// Returns memory cache, used for tests.
  const std::unique_ptr<InMemoryCache>& GetMemoryCache() const {
    return memory_cache_;
  };

  /// Returns mutable cache size, used for tests.
  uint64_t GetMutableCacheSize() const { return mutable_cache_data_size_; }

  /// Gets expiry key, used for tests.
  std::string GetExpiryKey(const std::string& key) const;

 private:
  /// Initializes LRU mutable cache if possible.
  void InitializeLru();

  /// Removes key from the mutable lru cache;
  void RemoveKeyLru(const std::string& key);

  /// Removes all keys with specified prefix from LRU mutable cache.
  void RemoveKeysWithPrefixLru(const std::string& key);

  /// Returns true if key is found in the LRU cache, false - otherwise.
  bool PromoteKeyLru(const std::string& key);

  /// Puts data into the mutable cache
  bool PutMutableCache(const std::string& key, const leveldb::Slice& value,
                       time_t expiry);

  DefaultCache::StorageOpenResult SetupStorage();

  boost::optional<std::pair<std::string, time_t>> GetFromDiscCache(
      const std::string& key);

  CacheSettings settings_;
  bool is_open_;
  std::unique_ptr<InMemoryCache> memory_cache_;
  std::unique_ptr<DiskCache> mutable_cache_;
  std::unique_ptr<DiskLruCache> mutable_cache_lru_;
  std::unique_ptr<DiskCache> protected_cache_;
  uint64_t mutable_cache_data_size_;
  std::mutex cache_lock_;
};

}  // namespace cache
}  // namespace olp
