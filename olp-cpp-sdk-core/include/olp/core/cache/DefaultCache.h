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
 * @brief Default cache that provides an in-memory LRU cache and persistance of
 * cached key,value pairs.
 */
class CORE_API DefaultCache : public KeyValueCache {
 public:
  /*! The storage open result type */
  enum StorageOpenResult {
    Success,            /*!< operation succeeded */
    OpenDiskPathFailure /*!< disk cache failure */
  };

  /**
   * @brief Parameterized constructor.
   * @param settings Settings for the cache.
   */
  DefaultCache(const CacheSettings& settings = CacheSettings());

  virtual ~DefaultCache();

  /**
   * @brief Open the cache to start read/write operations.
   *
   * @return StorageOpenResult if there were problems opening any of the
   * provided pathes on the disk.
   */
  StorageOpenResult Open();

  /**
   * @brief Closes the cache for use.
   */
  void Close();

  /**
   * @brief Clear the contents of the cache.
   * @return Returns true if the operation is successfull, false otherwise.
   */
  bool Clear();

  /**
   * @brief Store key,value pair into the cache
   * @param key Key for this value
   * @param value The value of any type
   * @param encoder A method provided to encode the specified value into a
   * string
   * @param expiry Time in seconds to when pair will expire
   * @return Returns true if the operation is successfull, false otherwise.
   */
  virtual bool Put(const std::string& key, const boost::any& value,
                   const Encoder& encoder, time_t expiry);

  /**
   * @brief Store raw binary data as value into the cache
   * @param key Key for this value
   * @param value binary data to be stored
   * @param expiry Time in seconds to when pair will expire
   * @return Returns true if the operation is successfull, false otherwise.
   */
  virtual bool Put(const std::string& key,
                   const std::shared_ptr<std::vector<unsigned char>> value,
                   time_t expiry);

  /**
   * @brief Get key,value pair from the cache
   * @param key Key to look for
   * @param decoder A method is provided to decode a value from a string if
   * needed
   */
  virtual boost::any Get(const std::string& key, const Decoder& encoder);

  /**
   * @brief Get key and binary data from the cache
   * @param key Key to look for
   */
  virtual std::shared_ptr<std::vector<unsigned char>> Get(
      const std::string& key);

  /**
   * @brief Remove a key,value pair from the cache
   * @param key Key for the value to remove from cache
   *
   * @return Returns true if the operation is successfull, false otherwise.
   */
  virtual bool Remove(const std::string& key);

  /**
   * @brief Remove values with keys matching the given prefix from the cache
   * @param prefix Prefix to look for
   *
   * @return Returns true on removal, false otherwise.
   */
  virtual bool RemoveKeysWithPrefix(const std::string& prefix);

 private:
  StorageOpenResult SetupStorage();

 private:
  CacheSettings settings_;
  bool is_open_{false};
  std::unique_ptr<InMemoryCache> memory_cache_;
  std::unique_ptr<DiskCache> mutable_cache_;
  std::unique_ptr<DiskCache> protected_cache_;
  std::mutex cache_lock_;
  // TODO: leveldb cache
};

}  // namespace cache
}  // namespace olp
