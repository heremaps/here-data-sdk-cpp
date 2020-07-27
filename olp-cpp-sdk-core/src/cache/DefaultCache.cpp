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

#include "DefaultCacheImpl.h"
#include "olp/core/cache/DefaultCache.h"
#include "olp/core/porting/warning_disable.h"

namespace olp {
namespace cache {

DefaultCache::DefaultCache(CacheSettings settings)
    : impl_(std::make_shared<DefaultCacheImpl>(std::move(settings))) {}

DefaultCache::~DefaultCache() = default;

DefaultCache::StorageOpenResult DefaultCache::Open() { return impl_->Open(); }

void DefaultCache::Close() { impl_->Close(); }

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

bool DefaultCache::RemoveKeysWithPrefix(const std::string& key) {
  return impl_->RemoveKeysWithPrefix(key);
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

bool DefaultCache::IsProtected(const std::string& key) {
  return impl_->IsProtected(key);
}

}  // namespace cache
}  // namespace olp
