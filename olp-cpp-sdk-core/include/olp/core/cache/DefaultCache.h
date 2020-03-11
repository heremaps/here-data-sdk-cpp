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

#include <memory>
#include <mutex>

#include "CacheSettings.h"
#include "KeyValueCache.h"

namespace olp {
namespace cache {

class InMemoryCache;
class DiskCache;

/**
 * @brief A default cache that provides an in-memory LRU cache and persistence
 * of cached key-value pairs.
 *
 * @note By default, the downloaded data is cached only in memory.
 * To enable the persistent cache, define
 * `olp::cache::CacheSettings::disk_path`. On iOS, the path is relative to
 * the application data folder.
 *
 * The default maximum size of the persistent cache is 32 MB. If the entire
 * available disk space should be used, set
 * `olp::cache::CacheSettings::max_disk_storage` to -1.
 * The implementation of the default persistent cache is based on
 * [LevelDB](https://github.com/google/leveldb). Due to the known LevelDB
 * limitation, the default cache can be accessed only by one process
 * exclusively.
 *
 * By default, the maximum size of the in-memory cache is 1MB. To change it,
 * set `olp::cache::CacheSettings::max_memory_cache_size` to the desired value.
 */
class CORE_API DefaultCache : public KeyValueCache {
 public:
  /*! The storage open result type */
  enum StorageOpenResult {
    Success,            /*!< The operation succeeded. */
    OpenDiskPathFailure /*!< The disk cache failure. */
  };

  /**
   * @brief Creates the `DefaultCache` instance.
   *
   * @param settings The cache settings.
   */
  explicit DefaultCache(CacheSettings settings = {});
  ~DefaultCache() override;

  /**
   * @brief Opens the cache to start read and write operations.
   *
   * @return `StorageOpenResult` if there are problems opening any of
   * the provided paths on the disk.
   */
  StorageOpenResult Open();

  /**
   * @brief Closes the cache.
   */
  void Close();

  /**
   * @brief Clears the cache content.
   *
   * @return True if the operation is successful; false otherwise.
   */
  bool Clear();

  /**
   * @brief Stores the key-value pair in the cache.
   *
   * @param key The key for this value.
   * @param value The value of any type.
   * @param encoder Encodes the specified value into a string.
   * @param expiry The expiry time (in seconds) of the key-value pair.
   *
   * @return True if the operation is successful; false otherwise.
   */
  bool Put(const std::string& key, const boost::any& value,
           const Encoder& encoder, time_t expiry) override;

  /**
   * @brief Stores the raw binary data as a value in the cache.
   *
   * @param key The key for this value.
   * @param value The binary data that should be stored.
   * @param expiry The expiry time (in seconds) of the key-value pair.
   *
   * @return True if the operation is successful; false otherwise.
   */
  bool Put(const std::string& key, const KeyValueCache::ValueTypePtr value,
           time_t expiry) override;

  /**
   * @brief Gets the key-value pair from the cache.
   *
   * @param key The key that is used to look for the key-value pair.
   * @param decoder Decodes the value from a string.
   *
   * @return The key-value pair.
   */
  boost::any Get(const std::string& key, const Decoder& decoder) override;

  /**
   * @brief Gets the key and binary data from the cache.
   *
   * @param key The key that is used to look for the binary data.
   *
   * @return The key and binary data.
   */
  KeyValueCache::ValueTypePtr Get(const std::string& key) override;

  /**
   * @brief Removes the key-value pair from the cache.
   *
   * @param key The key for the value.
   *
   * @return True if the operation is successful; false otherwise.
   */
  bool Remove(const std::string& key) override;

  /**
   * @brief Removes the values with the keys that match the given
   * prefix from the cache.
   *
   * @param prefix The prefix that matches the keys.
   *
   * @return True if the values are removed; false otherwise.
   */
  bool RemoveKeysWithPrefix(const std::string& prefix) override;

 private:
  StorageOpenResult SetupStorage();
  boost::optional<std::pair<std::string, time_t>> GetFromDiscCache(
      const std::string& key);

 private:
  CacheSettings settings_;
  bool is_open_;
  std::unique_ptr<InMemoryCache> memory_cache_;
  std::unique_ptr<DiskCache> mutable_cache_;
  std::unique_ptr<DiskCache> protected_cache_;
  std::mutex cache_lock_;
};

}  // namespace cache
}  // namespace olp
