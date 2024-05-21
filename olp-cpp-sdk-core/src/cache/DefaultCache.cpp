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

#include "olp/core/cache/DefaultCache.h"
#include "DefaultCacheImpl.h"

namespace olp {
namespace cache {

DefaultCache::DefaultCache(CacheSettings settings)
    : impl_(std::make_shared<DefaultCacheImpl>(std::move(settings))) {}

DefaultCache::~DefaultCache() = default;

DefaultCache::StorageOpenResult DefaultCache::Open() { return impl_->Open(); }

DefaultCache::StorageOpenResult DefaultCache::Open(CacheType type) {
  return impl_->Open(type);
}

void DefaultCache::Close() { impl_->Close(); }

bool DefaultCache::Close(CacheType type) { return impl_->Close(type); }

bool DefaultCache::Clear() { return impl_->Clear(); }

void DefaultCache::Compact() { return impl_->Compact(); }

bool DefaultCache::Put(const std::string& key, const boost::any& value,
                       const Encoder& encoder, time_t expiry) {
  return impl_->Put(key, value, encoder, expiry);
}

bool DefaultCache::Put(const std::string& key,
                       const KeyValueCache::ValueTypePtr value, time_t expiry) {
  return impl_->Put(key, value, expiry);
}

boost::any DefaultCache::Get(const std::string& key, const Decoder& decoder) {
  return impl_->Get(key, decoder);
}

KeyValueCache::ValueTypePtr DefaultCache::Get(const std::string& key) {
  return impl_->Get(key);
}

bool DefaultCache::Remove(const std::string& key) { return impl_->Remove(key); }

bool DefaultCache::RemoveKeysWithPrefix(const std::string& prefix) {
  return impl_->RemoveKeysWithPrefix(prefix);
}

bool DefaultCache::Contains(const std::string& key) const {
  return impl_->Contains(key);
}

bool DefaultCache::Protect(const KeyValueCache::KeyListType& keys) {
  return impl_->Protect(keys);
}

bool DefaultCache::Release(const KeyValueCache::KeyListType& keys) {
  return impl_->Release(keys);
}

bool DefaultCache::IsProtected(const std::string& key) const {
  return impl_->IsProtected(key);
}

uint64_t DefaultCache::Size(CacheType cache_type) const {
  return impl_->Size(cache_type);
}

uint64_t DefaultCache::Size(uint64_t new_size) { return impl_->Size(new_size); }

void DefaultCache::Promote(const std::string& key) { impl_->Promote(key); }

OperationOutcome<KeyValueCache::ValueTypePtr> DefaultCache::Read(
    const std::string& key) {
  return impl_->Read(key);
}

OperationOutcomeEmpty DefaultCache::Write(
    const std::string& key, const KeyValueCache::ValueTypePtr& value,
    time_t expiry) {
  return impl_->Write(key, value, expiry);
}

OperationOutcomeEmpty DefaultCache::Delete(const std::string& key) {
  return impl_->Delete(key);
}

OperationOutcomeEmpty DefaultCache::DeleteByPrefix(const std::string& prefix) {
  return impl_->DeleteByPrefix(prefix);
}

}  // namespace cache
}  // namespace olp
