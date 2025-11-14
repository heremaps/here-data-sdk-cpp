/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <ctime>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <olp/core/CoreApi.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/porting/any.h>
#include <olp/core/utils/WarningWorkarounds.h>

namespace olp {
namespace cache {

using Encoder = std::function<std::string()>;
using Decoder = std::function<olp::porting::any(const std::string&)>;

template <typename Result>
using OperationOutcome = client::ApiResponse<Result, client::ApiError>;
using OperationOutcomeEmpty = OperationOutcome<client::ApiNoResult>;

/// An interface for a cache that expects a key-value pair.
class CORE_API KeyValueCache {
 public:
  /**
   * @brief The expiry time of the key-value pair.
   *
   * By default, the key-value pair has no expiry time.
   */
  static constexpr time_t kDefaultExpiry = std::numeric_limits<time_t>::max();

  /// The value type that is stored in the DB.
  using ValueType = std::vector<unsigned char>;

  /// The shared pointer type of the DB entry.
  using ValueTypePtr = std::shared_ptr<ValueType>;

  /// An alias for the list of keys to be protected or released.
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
  virtual bool Put(const std::string& key, const olp::porting::any& value,
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
   * @param encoder Decodes the value from a string.
   *
   * @return The key-value pair.
   */
  virtual olp::porting::any Get(const std::string& key,
                                const Decoder& encoder) = 0;

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
   * @brief Checks if the key is in the cache.
   *
   * @param key The key for the value.
   *
   * @return True if the key is cached; false otherwise.
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

  /**
   * @brief Promotes a key in the cache LRU when applicable.
   *
   * @param key The key to promote in the cache LRU.
   */
  virtual void Promote(const std::string& key) { OLP_SDK_CORE_UNUSED(key); }

  /**
   * @brief Gets the binary data from the cache.
   *
   * @param key The key that is used to look for the binary data.
   *
   * @return The binary data or an error if the data could not be retrieved from
   * the cache.
   */
  virtual OperationOutcome<ValueTypePtr> Read(const std::string& key) {
    OLP_SDK_CORE_UNUSED(key);
    return client::ApiError(client::ErrorCode::Unknown, "Not implemented");
  }

  /**
   * @brief Stores the raw binary data as a value in the cache.
   *
   * @param key The key for this value.
   * @param value The binary data that should be stored.
   * @param expiry The expiry time (in seconds) of the key-value pair.
   *
   * @return An error if the data could not be written to the cache.
   */
  virtual OperationOutcomeEmpty Write(const std::string& key,
                                      const ValueTypePtr& value,
                                      time_t expiry = kDefaultExpiry) {
    OLP_SDK_CORE_UNUSED(key);
    OLP_SDK_CORE_UNUSED(value);
    OLP_SDK_CORE_UNUSED(expiry);
    return client::ApiError(client::ErrorCode::Unknown, "Not implemented");
  }

  /**
   * @brief Removes the key-value pair from the cache.
   *
   * @param key The key for the value.
   *
   * @return An error if the data could not be removed from the cache.
   */
  virtual OperationOutcomeEmpty Delete(const std::string& key) {
    OLP_SDK_CORE_UNUSED(key);
    return client::ApiError(client::ErrorCode::Unknown, "Not implemented");
  }

  /**
   * @brief Removes the values with the keys that match the given
   * prefix from the cache.
   *
   * @param prefix The prefix that matches the keys.
   *
   * @return An error if the data could not be removed from the cache.
   */
  virtual OperationOutcomeEmpty DeleteByPrefix(const std::string& prefix) {
    OLP_SDK_CORE_UNUSED(prefix);
    return client::ApiError(client::ErrorCode::Unknown, "Not implemented");
  }

  /**
   * @brief Lists the keys that match the given prefix.
   *
   * @param prefix The prefix that matches the keys.
   *
   * @return The collection of matched keys or an error. Empty collection if not
   * keys match the prefix.
   */
  virtual OperationOutcome<KeyListType> ListKeysWithPrefix(
      const std::string& prefix) {
    OLP_SDK_CORE_UNUSED(prefix);
    return client::ApiError(client::ErrorCode::Unknown, "Not implemented");
  }
};

}  // namespace cache
}  // namespace olp
