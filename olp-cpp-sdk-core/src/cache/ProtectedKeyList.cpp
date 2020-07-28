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

#include "ProtectedKeyList.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "olp/core/logging/Log.h"

namespace {
constexpr auto kLogTag = "ProtectedKeyList";
}  // namespace

namespace olp {
namespace cache {

ProtectedKeyList::ProtectedKeyList()
    : protected_data_(), size_written_(0), dirty_(false) {}

bool ProtectedKeyList::Deserialize(KeyValueCache::ValueTypePtr value) {
  if (!value) {
    return false;
  }
  std::stringbuf string_buf;
  string_buf.pubsetbuf(reinterpret_cast<char*>(value->data()), value->size());

  std::istream stream(&string_buf);

  for (std::string str; std::getline(stream, str, '\0');) {
    protected_data_.insert(str);
  }
  dirty_ = false;
  size_written_ = value->size();
  return true;
}

KeyValueCache::ValueTypePtr ProtectedKeyList::Serialize() {
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
  dirty_ = false;
  size_written_ = value->size();
  return value;
}

bool ProtectedKeyList::Protect(
    const KeyValueCache::KeyListType& keys,
    const ProtectedKeyChanged& change_key_to_protected) {
  auto was_updated = false;
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
    // add protected key
    auto res = protected_data_.insert(hint, key);
    if (res != hint) {
      dirty_ = true;
      was_updated = true;
      // notify that key now is protected
      change_key_to_protected(key);
    }
  }
  return was_updated;
}

bool ProtectedKeyList::Release(const KeyValueCache::KeyListType& keys) {
  auto result = false;
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
      // if key is equal of prefix for hint key, remove hint, notify that key is
      // not longer protected (need to notify for all keys for this prefix)
      while (hint != protected_data_.end() && IsEqualOrPrefix(key, *hint)) {
        hint = protected_data_.erase(hint);
        dirty_ = true;
        result = true;
      }
    }
  }
  return result;
}

bool ProtectedKeyList::IsProtected(const std::string& key) const {
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

bool ProtectedKeyList::IsPrefix(const std::string& prefix,
                                const std::string& key) const {
  return (key.size() > prefix.size() &&
          std::equal(prefix.begin(), prefix.end(), key.begin()));
}

bool ProtectedKeyList::IsEqualOrPrefix(const std::string& prefix,
                                       const std::string& key) const {
  return (key.size() >= prefix.size() &&
          std::equal(prefix.begin(), prefix.end(), key.begin()));
}

std::uint64_t ProtectedKeyList::Size() const { return size_written_; }

bool ProtectedKeyList::IsDirty() const { return dirty_; }

std::uint64_t ProtectedKeyList::Count() const { return protected_data_.size(); }

void ProtectedKeyList::Clear() { protected_data_.clear(); }

}  // namespace cache
}  // namespace olp
