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

#include <algorithm>

#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"

namespace {

constexpr auto kLogTag = "DefaultCache";
constexpr auto kExpirySuffix = "::expiry";
constexpr auto kMaxDiskSize = std::uint64_t(-1);

std::string CreateExpiryKey(const std::string& key) {
  return key + kExpirySuffix;
}

bool IsExpiryKey(const std::string& key) {
  auto pos = key.rfind(kExpirySuffix);
  return pos == 0;
}

time_t GetRemainingExpiryTime(const std::string& key,
                              olp::cache::DiskCache& disk_cache) {
  auto expiry_key = CreateExpiryKey(key);
  auto expiry = olp::cache::KeyValueCache::kDefaultExpiry;
  auto expiry_value = disk_cache.Get(expiry_key);
  if (expiry_value) {
    expiry = std::stol(*expiry_value);
    expiry -= olp::cache::InMemoryCache::DefaultTimeProvider()();
  }

  return expiry;
}

void PurgeDiskItem(const std::string& key, olp::cache::DiskCache& disk_cache) {
  auto expiry_key = CreateExpiryKey(key);
  disk_cache.Remove(key);
  disk_cache.Remove(expiry_key);
}

bool StoreExpiry(const std::string& key, olp::cache::DiskCache& disk_cache,
                 time_t expiry) {
  auto expiry_key = CreateExpiryKey(key);
  return disk_cache.Put(
      expiry_key,
      std::to_string(expiry +
                     olp::cache::InMemoryCache::DefaultTimeProvider()()));
}

}  // namespace

namespace olp {
namespace cache {

DefaultCacheImpl::DefaultCacheImpl(CacheSettings settings)
    : settings_(std::move(settings)),
      is_open_(false),
      memory_cache_(nullptr),
      mutable_cache_(nullptr),
      mutable_cache_lru_(nullptr),
      protected_cache_(nullptr) {}

DefaultCache::StorageOpenResult DefaultCacheImpl::Open() {
  std::lock_guard<std::mutex> lock(cache_lock_);
  is_open_ = true;
  return SetupStorage();
}

void DefaultCacheImpl::Close() {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return;
  }

  memory_cache_.reset();
  mutable_cache_.reset();
  mutable_cache_lru_.reset();
  protected_cache_.reset();
  is_open_ = false;
}

bool DefaultCacheImpl::Clear() {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    memory_cache_->Clear();
  }

  if (mutable_cache_lru_) {
    mutable_cache_lru_->Clear();
  }

  if (mutable_cache_) {
    if (!mutable_cache_->Clear()) {
      return false;
    }
  }

  return SetupStorage() == DefaultCache::StorageOpenResult::Success;
}

bool DefaultCacheImpl::Put(const std::string& key, const boost::any& value,
                           const Encoder& encoder, time_t expiry) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return false;
  }

  auto encoded_item = encoder();
  if (memory_cache_) {
    const auto size = encoded_item.size();
    const bool result = memory_cache_->Put(key, value, expiry, size);
    if (!result && size > settings_.max_memory_cache_size) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Failed to store value in memory cache %s, size %d",
                            key.c_str(), static_cast<int>(size));
    }
  }

  if (mutable_cache_) {
    if (expiry < KeyValueCache::kDefaultExpiry) {
      if (!StoreExpiry(key, *mutable_cache_, expiry)) {
        return false;
      }
    }

    if (!mutable_cache_->Put(key, encoded_item)) {
      return false;
    }

    if (mutable_cache_lru_) {
      const auto value = encoded_item.size();
      auto result = mutable_cache_lru_->InsertOrAssign(key, value);
      if (result.first == mutable_cache_lru_->end() && !result.second) {
        OLP_SDK_LOG_WARNING_F(
            kLogTag, "Failed to store value in mutable LRU cache, key %s",
            key.c_str());
        return false;
      }
    }
  }

  return true;
}

bool DefaultCacheImpl::Put(const std::string& key,
                           const KeyValueCache::ValueTypePtr value,
                           time_t expiry) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    const auto size = value->size();
    const bool result = memory_cache_->Put(key, value, expiry, size);
    if (!result && size > settings_.max_memory_cache_size) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Failed to store value in memory cache %s, size %d",
                            key.c_str(), static_cast<int>(size));
    }
  }

  if (mutable_cache_) {
    if (expiry < KeyValueCache::kDefaultExpiry) {
      if (!StoreExpiry(key, *mutable_cache_, expiry)) {
        return false;
      }
    }

    leveldb::Slice slice(reinterpret_cast<const char*>(value->data()),
                         value->size());
    if (!mutable_cache_->Put(key, slice)) {
      return false;
    }

    if (mutable_cache_lru_) {
      auto result = mutable_cache_lru_->InsertOrAssign(key, value->size());
      if (result.first == mutable_cache_lru_->end() && !result.second) {
        OLP_SDK_LOG_WARNING_F(
            kLogTag, "Failed to store value inmutable LRU cache, key %s",
            key.c_str());
        return false;
      }
    }
  }

  return true;
}

boost::any DefaultCacheImpl::Get(const std::string& key,
                                 const Decoder& decoder) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return boost::any();
  }

  PromoteKeyLru(key);

  if (memory_cache_) {
    auto value = memory_cache_->Get(key);
    if (!value.empty()) {
      return value;
    }
  }

  auto disc_cache = GetFromDiscCache(key);

  if (disc_cache) {
    auto decoded_item = decoder(disc_cache->first);
    if (memory_cache_) {
      memory_cache_->Put(key, decoded_item, disc_cache->second,
                         disc_cache->first.size());
    }
    return decoded_item;
  }

  return boost::any();
}

KeyValueCache::ValueTypePtr DefaultCacheImpl::Get(const std::string& key) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return nullptr;
  }

  PromoteKeyLru(key);

  if (memory_cache_) {
    auto value = memory_cache_->Get(key);

    if (!value.empty()) {
      return boost::any_cast<KeyValueCache::ValueTypePtr>(value);
    }
  }

  auto disc_cache = GetFromDiscCache(key);
  if (disc_cache) {
    const std::string& cached_data = disc_cache->first;
    auto data = std::make_shared<KeyValueCache::ValueType>(cached_data.size());

    std::memcpy(&data->front(), cached_data.data(), cached_data.size());

    if (memory_cache_) {
      memory_cache_->Put(key, data, disc_cache->second, data->size());
    }
    return data;
  }

  return nullptr;
}

bool DefaultCacheImpl::Remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    memory_cache_->Remove(key);
  }

  if (mutable_cache_lru_) {
    mutable_cache_lru_->Erase(key);
  }

  if (mutable_cache_) {
    if (!mutable_cache_->Remove(key)) {
      return false;
    }
  }

  return true;
}

bool DefaultCacheImpl::RemoveKeysWithPrefix(const std::string& key) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    memory_cache_->RemoveKeysWithPrefix(key);
  }

  RemoveKeysWithPrefixLru(key);

  if (mutable_cache_) {
    return mutable_cache_->RemoveKeysWithPrefix(key);
  }
  return true;
}

void DefaultCacheImpl::InitializeLru() {
  if (!mutable_cache_ || settings_.max_disk_storage == kMaxDiskSize ||
      settings_.eviction_policy == EvictionPolicy::kNone) {
    return;
  }

  mutable_cache_lru_ =
      std::make_unique<DiskLruCache>(settings_.max_disk_storage);
  auto it = mutable_cache_->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    const auto key = it->key().ToString();
    const auto& value = it->value();

    // Skip expiry keys
    if (IsExpiryKey(key)) {
      continue;
    }

    mutable_cache_lru_->Insert(key, value.size());
  }
}

void DefaultCacheImpl::RemoveKeysWithPrefixLru(const std::string& key) {
  if (!mutable_cache_lru_) {
    return;
  }

  for (auto it = mutable_cache_lru_->begin();
       it != mutable_cache_lru_->end();) {
    auto const& element_key = it.key();
    if (element_key.size() >= key.size() &&
        std::equal(key.begin(), key.end(), element_key.begin())) {
      it = mutable_cache_lru_->Erase(it);
      continue;
    }

    ++it;
  }
}

bool DefaultCacheImpl::PromoteKeyLru(const std::string& key) {
  if (mutable_cache_lru_) {
    auto it = mutable_cache_lru_->Find(key);
    return it != mutable_cache_lru_->end();
  }

  return true;
}

DefaultCache::StorageOpenResult DefaultCacheImpl::SetupStorage() {
  auto result = DefaultCache::Success;

  memory_cache_.reset();
  mutable_cache_.reset();
  mutable_cache_lru_.reset();
  protected_cache_.reset();

  if (settings_.max_memory_cache_size > 0) {
    memory_cache_.reset(new InMemoryCache(settings_.max_memory_cache_size));
  }

  if (settings_.disk_path_mutable) {
    StorageSettings storage_settings;
    storage_settings.max_disk_storage = settings_.max_disk_storage;
    storage_settings.max_chunk_size = settings_.max_chunk_size;
    storage_settings.enforce_immediate_flush =
        settings_.enforce_immediate_flush;
    storage_settings.max_file_size = settings_.max_file_size;

    mutable_cache_ = std::make_unique<DiskCache>();
    auto status = mutable_cache_->Open(settings_.disk_path_mutable.get(),
                                       settings_.disk_path_mutable.get(),
                                       storage_settings, OpenOptions::Default);
    if (status == OpenResult::Fail) {
      OLP_SDK_LOG_ERROR_F(kLogTag, "Failed to open the mutable cache %s",
                          settings_.disk_path_mutable.get().c_str());

      mutable_cache_.reset();
      settings_.disk_path_mutable = boost::none;
      result = DefaultCache::OpenDiskPathFailure;
    }
  }

  InitializeLru();

  if (settings_.disk_path_protected) {
    protected_cache_ = std::make_unique<DiskCache>();
    auto status =
        protected_cache_->Open(settings_.disk_path_protected.get(),
                               settings_.disk_path_protected.get(),
                               StorageSettings{}, OpenOptions::ReadOnly);
    if (status == OpenResult::Fail) {
      OLP_SDK_LOG_ERROR_F(kLogTag, "Failed to reopen protected cache %s",
                          settings_.disk_path_protected.get().c_str());

      protected_cache_.reset();
      settings_.disk_path_protected = boost::none;
      result = DefaultCache::OpenDiskPathFailure;
    }
  }

  return result;
}

boost::optional<std::pair<std::string, time_t>>
DefaultCacheImpl::GetFromDiscCache(const std::string& key) {
  if (protected_cache_) {
    auto result = protected_cache_->Get(key);
    if (result) {
      auto default_expiry = KeyValueCache::kDefaultExpiry;
      return std::make_pair(std::move(result.value()), default_expiry);
    }
  }

  if (mutable_cache_) {
    auto expiry = GetRemainingExpiryTime(key, *mutable_cache_);
    if (expiry <= 0) {
      PurgeDiskItem(key, *mutable_cache_);
      if (mutable_cache_lru_) {
        mutable_cache_lru_->Erase(key);
      }
    } else {
      if (!PromoteKeyLru(key)) {
        // If not found in LRU no need to look in disk cache either.
        return boost::none;
      }

      auto result = mutable_cache_->Get(key);
      if (result) {
        return std::make_pair(std::move(result.value()), expiry);
      }
    }
  }
  return boost::none;
}

}  // namespace cache
}  // namespace olp
