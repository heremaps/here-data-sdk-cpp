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

#include <boost/any.hpp>

#include <olp/core/CoreApi.h>

namespace olp {

namespace cache {

using Encoder = std::function<std::string()>;
using Decoder = std::function<boost::any(const std::string&)>;

/**
 * @brief Interface for cache expecting a key,value pair.
 */
class CORE_API KeyValueCache {
 public:
  virtual ~KeyValueCache() = default;

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
                   const Encoder& encoder,
                   time_t expiry = (std::numeric_limits<time_t>::max)()) = 0;

  /**
   * @brief Store raw binary data as value into the cache
   * @param key Key for this value
   * @param value binary data to be stored
   * @param expiry Time in seconds to when pair will expire
   * @return Returns true if the operation is successfull, false otherwise.
   */
  virtual bool Put(const std::string& key,
                   const std::shared_ptr<std::vector<unsigned char>> value,
                   time_t expiry = (std::numeric_limits<time_t>::max)()) = 0;

  /**
   * @brief Get key,value pair from the cache
   * @param key Key to look for
   * @param decoder A method is provided to decode a value from a string if
   * needed
   */
  virtual boost::any Get(const std::string& key, const Decoder& encoder) = 0;

  /**
   * @brief Get key and binary data from the cache
   * @param key Key to look for
   */
  virtual std::shared_ptr<std::vector<unsigned char>> Get(
      const std::string& key) = 0;

  /**
   * @brief Remove a key,value pair from the cache
   * @param key Key for the value to remove from cache
   *
   * @return Returns true if the operation is successfull, false otherwise.
   */
  virtual bool Remove(const std::string& key) = 0;

  /**
   * @brief Remove values with keys matching the given prefix from the cache
   * @param prefix Prefix to look for
   *
   * @return Returns true on removal, false otherwise.
   */
  virtual bool RemoveKeysWithPrefix(const std::string& prefix) = 0;
};

}  // namespace cache

}  // namespace olp
