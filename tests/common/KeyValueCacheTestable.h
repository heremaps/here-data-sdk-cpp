/*
 * Copyright (C) 2021-2024 HERE Europe B.V.
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

class KeyValueCacheTestable : public olp::cache::KeyValueCache {
 public:
  explicit KeyValueCacheTestable(
      std::shared_ptr<olp::cache::KeyValueCache> base_cache)
      : base_cache_{std::move(base_cache)} {}

  bool Put(const std::string& key, const olp::porting::any& value,
           const olp::cache::Encoder& encoder, time_t expiry) override {
    return base_cache_->Put(key, value, encoder, expiry);
  }

  bool Put(const std::string& key, const ValueTypePtr value,
           time_t expiry) override {
    return base_cache_->Put(key, value, expiry);
  }

  olp::porting::any Get(const std::string& key,
                        const olp::cache::Decoder& encoder) override {
    return base_cache_->Get(key, encoder);
  }

  ValueTypePtr Get(const std::string& key) override {
    return base_cache_->Get(key);
  }

  bool Remove(const std::string& key) override {
    return base_cache_->Remove(key);
  }

  bool RemoveKeysWithPrefix(const std::string& prefix) override {
    return base_cache_->RemoveKeysWithPrefix(prefix);
  }

  bool Contains(const std::string& key) const override {
    return base_cache_->Contains(key);
  }

  bool Protect(const KeyListType& keys) override {
    return base_cache_->Protect(keys);
  }

  bool Release(const KeyListType& keys) override {
    return base_cache_->Release(keys);
  }

  bool IsProtected(const std::string& key) const override {
    return base_cache_->IsProtected(key);
  }

  olp::cache::OperationOutcome<KeyValueCache::ValueTypePtr> Read(
      const std::string& key) override {
    return base_cache_->Read(key);
  }

  olp::cache::OperationOutcomeEmpty Write(
      const std::string& key, const KeyValueCache::ValueTypePtr& value,
      time_t expiry) override {
    return base_cache_->Write(key, value, expiry);
  }

  olp::cache::OperationOutcomeEmpty Delete(const std::string& key) override {
    return base_cache_->Delete(key);
  }

  olp::cache::OperationOutcomeEmpty DeleteByPrefix(
      const std::string& prefix) override {
    return base_cache_->DeleteByPrefix(prefix);
  }

 protected:
  std::shared_ptr<olp::cache::KeyValueCache> base_cache_;
};

struct CacheWithPutErrors : public KeyValueCacheTestable {
  using KeyValueCacheTestable::KeyValueCacheTestable;

  bool Put(const std::string&, const olp::porting::any&,
           const olp::cache::Encoder&, time_t) override {
    return false;
  }

  bool Put(const std::string&, const ValueTypePtr, time_t) override {
    return false;
  }

  olp::cache::OperationOutcomeEmpty Write(const std::string&,
                                          const KeyValueCache::ValueTypePtr&,
                                          time_t) override {
    return olp::client::ApiError::CacheIO();
  }
};
