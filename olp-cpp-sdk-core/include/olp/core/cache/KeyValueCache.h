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
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/utils/WarningWorkarounds.h>
#include <boost/any.hpp>

namespace olp {
namespace cache {

using Encoder = std::function<std::string()>;
using Decoder = std::function<boost::any(const std::string&)>;

/**
 * @brief An interface for a cache that expects a key-value pair.
 */
class CORE_API KeyValueCache {
 public:
  /**
   * @brief The expiry time of the key-value pair.
   *
   * By default, the key-value pair has no expiry time.
   */
  static constexpr time_t kDefaultExpiry = std::numeric_limits<time_t>::max();

  /**
   * @brief The value type that is stored in the DB.
   */
  using ValueType = std::vector<unsigned char>;

  /**
   * @brief The shared pointer type of the DB entry.
   */
  using ValueTypePtr = std::shared_ptr<ValueType>;

  /**
   * @brief Alias for the list of keys to be protected or released.
   */
  using KeyListType = std::vector<std::string>;

  virtual ~KeyValueCache() = default;

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
  virtual bool Put(const std::string& key, const boost::any& value,
                   const Encoder& encoder, time_t expiry = kDefaultExpiry) = 0;

  /**
   * @brief Stores the raw binary data as a value in the cache.
   *
   * @param key The key for this value.
   * @param value The binary data that should be stored.
   * @param expiry The expiry time (in seconds) of the key-value pair.
   *
   * @return True if the operation is successful; false otherwise.
   */
  virtual bool Put(const std::string& key, const ValueTypePtr value,
                   time_t expiry = kDefaultExpiry) = 0;

  /**
   * @brief Gets the key-value pair from the cache.
   *
   * @param key The key that is used to look for the key-value pair.
   * @param decoder Decodes the value from a string.
   *
   * @return The key-value pair.
   */
  virtual boost::any Get(const std::string& key, const Decoder& encoder) = 0;

  /**
   * @brief Gets the key and binary data from the cache.
   *
   * @param key The key that is used to look for the binary data.
   *
   * @return The key and binary data.
   */
  virtual ValueTypePtr Get(const std::string& key) = 0;

  /**
   * @brief Removes the key-value pair from the cache.
   *
   * @param key The key for the value.
   *
   * @return True if the operation is successful; false otherwise.
   */
  virtual bool Remove(const std::string& key) = 0;

  /**
   * @brief Removes the values with the keys that match the given
   * prefix from the cache.
   *
   * @param prefix The prefix that matches the keys.
   *
   * @return True if the values are removed; false otherwise.
   */
  virtual bool RemoveKeysWithPrefix(const std::string& prefix) = 0;

  /**
   * @brief Check if key is in the cache.
   *
   * @param key The key for the value.
   *
   * @return True if the key/value cached ; false otherwise.
   */
  virtual bool Contains(const std::string& key) const {
    OLP_SDK_CORE_UNUSED(key);
    return false;
  }

  /**
   * @brief Protects keys from eviction.
   *
   * You can use keys or prefixes to protect single keys or entire catalogs,
   * layers, or version.
   *
   * @param keys The list of keys or prefixes.
   *
   * @return True if the keys are added to the protected list; false otherwise.
   */
  virtual bool Protect(const KeyListType& keys) {
    OLP_SDK_CORE_UNUSED(keys);
    return false;
  }

  /**
   * @brief Removes a list of keys from protection.
   *
   * The provided keys can be full keys or prefixes only.
   *
   * @param keys The list of keys or prefixes.
   *
   * @return True if the keys are removed from the protected list; false
   * otherwise.
   */
  virtual bool Release(const KeyListType& keys) {
    OLP_SDK_CORE_UNUSED(keys);
    return false;
  }

  /**
   * @brief Checks if key is protected.
   *
   * @param key The key or prefix.
   *
   * @return True if the key are in the protected list; false
   * otherwise.
   */
  virtual bool IsProtected(const std::string& key) const {
    OLP_SDK_CORE_UNUSED(key);
    return false;
  }
};

}  // namespace cache
}  // namespace olp
