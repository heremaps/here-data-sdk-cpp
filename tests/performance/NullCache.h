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

#include <olp/core/cache/KeyValueCache.h>

/*
 * Class designed to do nothing. Used by memory test, to gather the SDK
 * allocations and memory consumption.
 */
class NullCache : public olp::cache::KeyValueCache {
 public:
  bool Put(const std::string& /*key*/, const boost::any& /*value*/,
           const olp::cache::Encoder& /*encoder*/, time_t /*expiry*/) override {
    return false;
  }

  bool Put(const std::string& /*key*/,
           const std::shared_ptr<std::vector<unsigned char>> /*value*/,
           time_t /*expiry*/) override {
    return false;
  }

  boost::any Get(const std::string& /*key*/,
                 const olp::cache::Decoder& /*encoder*/) override {
    return {};
  }

  std::shared_ptr<std::vector<unsigned char>> Get(
      const std::string& /*key*/) override {
    return nullptr;
  }

  bool Remove(const std::string& /*key*/) override { return true; }

  bool RemoveKeysWithPrefix(const std::string& /*prefix*/) override {
    return true;
  }
};
