/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "ProtectedData.h"

#include <algorithm>
#include <string>
#include <utility>

#include "olp/core/logging/Log.h"

namespace {
constexpr auto kLogTag = "ProtectedData";
}  // namespace

namespace olp {
namespace cache {

bool ProtectedData::Deserialize(KeyValueCache::ValueTypePtr value) {
  auto result = true;
  std::string str;
  for (const auto& el : *value) {
    if (el == '\0' && !str.empty()) {
      protected_data_.insert(str);
      str.clear();
    } else {
      str += el;
    }
  }
  list_updated_ = false;
  size_written_ = value->size();
  return result;
}

KeyValueCache::ValueTypePtr ProtectedData::Serialize() {
  auto value = std::make_shared<std::vector<unsigned char>>();
  auto size = 0;
  for (const auto& key : protected_data_) {
    size += key.length() + 1;
  }
  if (size > 0) {
    value->reserve(size);

    for (const auto& key : protected_data_) {
      value->insert(value->end(), key.begin(), key.end());
      if (key.back() != '\0') {
        value->emplace_back('\0');
      }
    }
  }
  list_updated_ = false;
  size_written_ = value->size();
  return value;
}

bool ProtectedData::Protect(
    const DefaultCache::KeyListType& keys,
    const ProtectedKeyChanged& change_key_to_protected) {
  list_updated_ = true;
  for (const auto& key : keys) {
    // find key or prefix
    auto hint = protected_data_.lower_bound(key);
    if (hint != protected_data_.end()) {
      // if hint is prefix for this key, ignore this key, it is allready
      // protected
      if (IsPrefix(*hint, key)) {
        continue;
      }
      // if key is prefix for hint key, remove hint and add prefix
      while (hint != protected_data_.end() && IsPrefix(key, *hint)) {
        hint = protected_data_.erase(hint);
      }
    }

    // remove keys from lru and memory cache
    change_key_to_protected(key);

    // add protected key
    protected_data_.insert(hint, key);
  }
  return true;
}

bool ProtectedData::Release(
    const DefaultCache::KeyListType& keys,
    const ProtectedKeyChanged& released_key_from_protected) {
  list_updated_ = true;
  auto result = true;
  for (const auto& key : keys) {
    // find key or prefix
    auto hint = protected_data_.lower_bound(key);
    if (hint != protected_data_.end()) {
      // if hint is prefix for this key, it is allready protected, could not
      // unprotect one key, return error
      if (IsPrefix(*hint, key)) {
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "Prefix is stored for key='%s', prefix='%s'",
                              key.c_str(), (*hint).c_str());
        result = false;
        break;
      }
      // if key is equal of prefix for hint key, remove hint, add key to
      // lru(need to release all keys for this prefix)
      while (hint != protected_data_.end() && IsEqualOrPrefix(key, *hint)) {
        released_key_from_protected(*hint);
        hint = protected_data_.erase(hint);
      }
    }
  }
  return result;
}

bool ProtectedData::IsProtected(const std::string& key) const {
  auto it = protected_data_.lower_bound(key);
  if (it == protected_data_.end()) {
    return false;
  }
  // need to check if keys equal, but only case when prefix stored, not
  // othervise
  if (key.length() < (*it).length()) {
    return false;
  }
  // check if we store key or its prefix
  if (IsEqualOrPrefix(*it, key)) {
    return true;
  }
  return false;
}

bool ProtectedData::IsPrefix(const std::string& prefix,
                             const std::string& key) const {
  return (key.size() > prefix.size() &&
          std::equal(prefix.begin(), prefix.end(), key.begin()));
}

bool ProtectedData::IsEqualOrPrefix(const std::string& prefix,
                                    const std::string& key) const {
  return (key.size() >= prefix.size() &&
          std::equal(prefix.begin(), prefix.end(), key.begin()));
}

std::uint64_t ProtectedData::GetWrittenListSize() const {
  return size_written_;
}

bool ProtectedData::IsListDirty() const { return list_updated_; }

}  // namespace cache
}  // namespace olp
