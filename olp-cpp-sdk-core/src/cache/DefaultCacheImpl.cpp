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
#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"

namespace {

constexpr auto kLogTag = "DefaultCache";
constexpr auto kExpirySuffix = "::expiry";
constexpr auto kProtectedKeys = "internal::protected::protected_data";
constexpr auto kInternalKeysPrefix = "internal::";
constexpr auto kMaxDiskSize = std::uint64_t(-1);
constexpr auto kMinDiskUsedThreshold = 0.85f;
constexpr auto kMaxDiskUsedThreshold = 0.9f;

// current epoch time contains 10 digits.
constexpr auto kExpiryValueSize = 10;
const auto kExpirySuffixLength = strlen(kExpirySuffix);

std::string CreateExpiryKey(const std::string& key) {
  return key + kExpirySuffix;
}

bool IsExpiryKey(const std::string& key) {
  return key.rfind(kExpirySuffix) != std::string::npos;
}

bool IsExpiryValid(time_t expiry) {
  return expiry < olp::cache::KeyValueCache::kDefaultExpiry;
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

void PurgeDiskItem(const std::string& key, olp::cache::DiskCache& disk_cache,
                   uint64_t& removed_data_size) {
  auto expiry_key = CreateExpiryKey(key);
  uint64_t data_size = 0u;

  disk_cache.Remove(key, data_size);
  removed_data_size += data_size;

  disk_cache.Remove(expiry_key, data_size);
  removed_data_size += data_size;
}

size_t StoreExpiry(const std::string& key, leveldb::WriteBatch& batch,
                   time_t expiry) {
  auto expiry_key = CreateExpiryKey(key);
  auto time_str = std::to_string(expiry);
  batch.Put(expiry_key, time_str);

  return expiry_key.size() + time_str.size();
}

leveldb::CompressionType GetCompression(
    olp::cache::CompressionType compression) {
  return (compression == olp::cache::CompressionType::kNoCompression)
             ? leveldb::kNoCompression
             : leveldb::kSnappyCompression;
}

int64_t GetElapsedTime(std::chrono::steady_clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start)
      .count();
}

bool IsInternalKey(const std::string& key) {
  return key.find(kInternalKeysPrefix) == 0u;
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
      protected_cache_(nullptr),
      mutable_cache_data_size_(0) {}

DefaultCache::StorageOpenResult DefaultCacheImpl::Open() {
  std::lock_guard<std::mutex> lock(cache_lock_);
  is_open_ = true;
  return SetupStorage();
}

DefaultCacheImpl::~DefaultCacheImpl() { Close(); }

void DefaultCacheImpl::Close() {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return;
  }
  if (mutable_cache_ && protected_keys_.IsDirty()) {
    auto batch = std::make_unique<leveldb::WriteBatch>();
    MaybeUpdatedProtectedKeys(*batch);
    auto result = mutable_cache_->ApplyBatch(std::move(batch));
    OLP_SDK_LOG_INFO_F(kLogTag,
                       "Close(): store list of protected keys, result=%s",
                       result.IsSuccessful() ? "true" : "false");
  }

  memory_cache_.reset();
  mutable_cache_.reset();
  mutable_cache_lru_.reset();
  protected_cache_.reset();
  mutable_cache_data_size_ = 0;
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
    mutable_cache_data_size_ = 0;
    if (!mutable_cache_->Clear()) {
      return false;
    }
  }

  return SetupStorage() == DefaultCache::StorageOpenResult::Success;
}

void DefaultCacheImpl::Compact() {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (mutable_cache_) {
    mutable_cache_->Compact();
  }
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
    const bool result = memory_cache_->Put(
        key, value, GetExpiryForMemoryCache(key, expiry), size);
    if (!result && size > settings_.max_memory_cache_size) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Failed to store value in memory cache %s, size %d",
                            key.c_str(), static_cast<int>(size));
    }
  }

  return PutMutableCache(key, encoded_item, expiry);
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
    const bool result = memory_cache_->Put(
        key, value, GetExpiryForMemoryCache(key, expiry), size);
    if (!result && size > settings_.max_memory_cache_size) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Failed to store value in memory cache %s, size %d",
                            key.c_str(), static_cast<int>(size));
    }
  }

  leveldb::Slice slice(reinterpret_cast<const char*>(value->data()),
                       value->size());
  return PutMutableCache(key, slice, expiry);
}

boost::any DefaultCacheImpl::Get(const std::string& key,
                                 const Decoder& decoder) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return boost::any();
  }

  if (memory_cache_) {
    auto value = memory_cache_->Get(key);
    if (!value.empty()) {
      PromoteKeyLru(key);
      return value;
    }
  }

  auto disc_cache = GetFromDiscCache(key);

  if (disc_cache) {
    auto decoded_item = decoder(disc_cache->first);
    if (memory_cache_) {
      auto expiry = disc_cache->second;
      memory_cache_->Put(key, decoded_item,
                         GetExpiryForMemoryCache(key, expiry),
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

  if (memory_cache_) {
    auto value = memory_cache_->Get(key);
    if (!value.empty()) {
      PromoteKeyLru(key);
      return boost::any_cast<KeyValueCache::ValueTypePtr>(value);
    }
  }

  KeyValueCache::ValueTypePtr value = nullptr;
  time_t expiry = KeyValueCache::kDefaultExpiry;

  auto result = GetFromDiskCache(key, value, expiry);
  if (result && value) {
    if (memory_cache_) {
      memory_cache_->Put(key, value, GetExpiryForMemoryCache(key, expiry),
                         value->size());
    }

    return value;
  }

  return nullptr;
}

bool DefaultCacheImpl::Remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return false;
  }
  // protected data could be removed by user
  if (memory_cache_) {
    memory_cache_->Remove(key);
  }

  RemoveKeyLru(key);

  if (mutable_cache_) {
    uint64_t removed_data_size = 0;
    PurgeDiskItem(key, *mutable_cache_, removed_data_size);

    mutable_cache_data_size_ -= removed_data_size;
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
    uint64_t removed_data_size = 0;
    auto result = mutable_cache_->RemoveKeysWithPrefix(key, removed_data_size);
    mutable_cache_data_size_ -= removed_data_size;
    return result;
  }
  return true;
}

bool DefaultCacheImpl::Contains(const std::string& key) const {
  std::lock_guard<std::mutex> lock(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_ && memory_cache_->Contains(key)) {
    return true;
  }

  // if lru exist check if key is there
  if (mutable_cache_lru_) {
    auto it = mutable_cache_lru_->FindNoPromote(key);
    if (it != mutable_cache_lru_->end()) {
      ValueProperties props = it->value();
      props.expiry -= olp::cache::InMemoryCache::DefaultTimeProvider()();
      return (props.expiry > 0);
      // if lru exist, but key not found, this case possible only for protected
      // keys
    } else if (protected_keys_.IsProtected(key)) {
      return mutable_cache_ && mutable_cache_->Contains(key);
    }

    // check in mutable cache only if lru does not exist
  } else if (mutable_cache_ && mutable_cache_->Contains(key)) {
    return (GetRemainingExpiryTime(key, *mutable_cache_) > 0) ||
           protected_keys_.IsProtected(key);
  }

  if (protected_cache_ && protected_cache_->Contains(key)) {
    return (GetRemainingExpiryTime(key, *protected_cache_) > 0);
  }

  return false;
}

bool DefaultCacheImpl::AddKeyLru(std::string key, const leveldb::Slice& value) {
  // do not add protected keys to lru, this applies to all keys with some
  // protected prefix, do not add internal keys
  if (mutable_cache_lru_ && !protected_keys_.IsProtected(key) &&
      !IsInternalKey(key)) {
    // remove the prefix to restore original key
    const bool expiration_key = IsExpiryKey(key);
    if (expiration_key) {
      key.resize(key.size() - kExpirySuffixLength);
    }

    ValueProperties props;

    auto iterator = mutable_cache_lru_->FindNoPromote(key);
    if (iterator != mutable_cache_lru_->end()) {
      props = iterator->value();
    }

    if (expiration_key) {
      // value.data() could point to a value without null character at the
      // end, this could cause exception in std::stol. This is fixed by
      // constructing a string, (We rely on small string optimization here).
      std::string timestamp(value.data(), value.size());
      props.expiry = std::stol(timestamp.c_str());
    } else {
      props.size = value.size();
    }

    auto result = mutable_cache_lru_->InsertOrAssign(key, props);
    return result.second;
  }
  return false;
}

void DefaultCacheImpl::InitializeLru() {
  if (!mutable_cache_) {
    return;
  }
  mutable_cache_data_size_ = 0;
  if (mutable_cache_ && settings_.max_disk_storage != kMaxDiskSize &&
      settings_.eviction_policy == EvictionPolicy::kLeastRecentlyUsed) {
    mutable_cache_lru_ =
        std::make_unique<DiskLruCache>(settings_.max_disk_storage);
    OLP_SDK_LOG_INFO_F(kLogTag, "Initializing mutable lru cache.");
  }

  const auto start = std::chrono::steady_clock::now();
  auto count = 0u;
  auto it = mutable_cache_->NewIterator(leveldb::ReadOptions());

  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    auto key = it->key().ToString();
    const auto& value = it->value();

    // Here we count both expiry keys and regular keys
    mutable_cache_data_size_ += key.size() + value.size();
    if (AddKeyLru(key, value)) {
      ++count;
    }
  }

  OLP_SDK_LOG_INFO_F(
      kLogTag, "Cache initialized, items=%" PRIu32 ", time=%" PRId64 " ms",
      count, GetElapsedTime(start));
}

bool DefaultCacheImpl::RemoveKeyLru(const std::string& key) {
  if (mutable_cache_lru_) {
    return mutable_cache_lru_->Erase(key);
  }
  return false;
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
    return it != mutable_cache_lru_->end() || protected_keys_.IsProtected(key);
  }

  return true;
}

uint64_t DefaultCacheImpl::MaybeEvictData(leveldb::WriteBatch& batch) {
  if (!mutable_cache_ || !mutable_cache_lru_) {
    return 0;
  }

  const auto max_size = kMaxDiskUsedThreshold * settings_.max_disk_storage;
  if (mutable_cache_data_size_ < max_size) {
    return 0;
  }

  const auto start = std::chrono::steady_clock::now();
  int64_t evicted = 0u;
  auto count = 0u;
  const auto min_size = kMinDiskUsedThreshold * settings_.max_disk_storage;

  const auto current_time = olp::cache::InMemoryCache::DefaultTimeProvider()();

  // Remove the expired elements first
  // protected elements are not stored in lru, so do not need to check
  for (auto it = mutable_cache_lru_->begin();
       it != mutable_cache_lru_->end() &&
       mutable_cache_data_size_ - evicted > min_size;) {
    const auto& key = it->key();
    const auto& properties = it->value();

    const bool expired = (properties.expiry - current_time) <= 0;

    if (!expired) {
      ++it;
      continue;
    }

    // Remove the key
    batch.Delete(key);
    evicted += key.size() + properties.size;

    // Remove the key's expiry
    auto expiry_key = CreateExpiryKey(key);
    batch.Delete(expiry_key);
    evicted += expiry_key.size() + kExpiryValueSize;

    ++count;

    if (memory_cache_) {
      memory_cache_->Remove(key);
    }

    it = mutable_cache_lru_->Erase(it);
  }

  // Remove the other elements if needed
  for (auto it = mutable_cache_lru_->rbegin();
       it != mutable_cache_lru_->rend() &&
       mutable_cache_data_size_ - evicted > min_size;) {
    const auto& key = it->key();
    const auto& properties = it->value();

    evicted += key.size() + properties.size;
    batch.Delete(key);

    if (IsExpiryValid(properties.expiry)) {
      const auto expiry_key = CreateExpiryKey(key);
      evicted += expiry_key.size() + kExpiryValueSize;
      batch.Delete(expiry_key);
    }

    ++count;

    if (memory_cache_) {
      memory_cache_->Remove(it->key());
    }

    mutable_cache_lru_->Erase(it);
    it = mutable_cache_lru_->rbegin();
  }

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "Evicted from mutable cache, items=%" PRId32
                     ", time=%" PRId64 "ms, size=%" PRIu64,
                     count, GetElapsedTime(start), evicted);

  return evicted;
}

int64_t DefaultCacheImpl::MaybeUpdatedProtectedKeys(
    leveldb::WriteBatch& batch) {
  if (protected_keys_.IsDirty()) {
    auto prev_size = protected_keys_.Size();
    auto value = protected_keys_.Serialize();
    if (value->size() > 0) {
      leveldb::Slice slice(reinterpret_cast<const char*>(value->data()),
                           value->size());
      batch.Put(kProtectedKeys, slice);
    }  // add key size, as it is be written for the first time
    auto key_size = (prev_size > 0 ? 0 : strlen(kProtectedKeys));
    return (key_size + protected_keys_.Size() - prev_size);
  }

  return 0;
}

bool DefaultCacheImpl::PutMutableCache(const std::string& key,
                                       const leveldb::Slice& value,
                                       time_t expiry) {
  if (!mutable_cache_) {
    return true;
  }

  // can't put new item if cache is full and eviction disabled
  const auto item_size = value.size();
  const auto expected_size = mutable_cache_data_size_ + item_size + key.size() +
                             key.size() + kExpirySuffixLength +
                             kExpiryValueSize;
  if (!mutable_cache_lru_ && expected_size > settings_.max_disk_storage) {
    return false;
  }

  uint64_t added_data_size = 0u;
  auto batch = std::make_unique<leveldb::WriteBatch>();
  batch->Put(key, value);
  added_data_size += key.size() + item_size;

  if (IsExpiryValid(expiry)) {
    expiry += olp::cache::InMemoryCache::DefaultTimeProvider()();
    added_data_size += StoreExpiry(key, *batch, expiry);
  }

  auto removed_data_size = MaybeEvictData(*batch);
  auto updated_data_size = MaybeUpdatedProtectedKeys(*batch);

  auto result = mutable_cache_->ApplyBatch(std::move(batch));
  if (!result.IsSuccessful()) {
    return false;
  }
  mutable_cache_data_size_ += added_data_size;
  mutable_cache_data_size_ -= removed_data_size;
  mutable_cache_data_size_ += updated_data_size;

  // do not add protected keys to lru
  if (mutable_cache_lru_ && !protected_keys_.IsProtected(key)) {
    ValueProperties props;
    props.size = item_size;
    props.expiry = expiry;
    const auto result = mutable_cache_lru_->InsertOrAssign(key, props);
    if (result.first == mutable_cache_lru_->end() && !result.second) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "Failed to store value in mutable LRU cache, key %s",
          key.c_str());
      return false;
    }
  }

  return true;
}

DefaultCache::StorageOpenResult DefaultCacheImpl::SetupStorage() {
  auto result = DefaultCache::Success;

  memory_cache_.reset();
  mutable_cache_.reset();
  mutable_cache_lru_.reset();
  protected_cache_.reset();
  mutable_cache_data_size_ = 0;

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
    storage_settings.compression = GetCompression(settings_.compression);

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
    } else {
      // read protected keys
      KeyValueCache::ValueTypePtr value = nullptr;
      auto result = mutable_cache_->Get(kProtectedKeys, value);
      if (result && value) {
        if (!protected_keys_.Deserialize(value)) {
          OLP_SDK_LOG_WARNING(kLogTag, "Deserialize protected keys failed");
        }
      }
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

bool DefaultCacheImpl::GetFromDiskCache(const std::string& key,
                                        KeyValueCache::ValueTypePtr& value,
                                        time_t& expiry) {
  // Make sure we do not get a dirty entry
  value = nullptr;
  expiry = KeyValueCache::kDefaultExpiry;

  if (protected_cache_) {
    auto result = protected_cache_->Get(key, value);
    if (result && value && !value->empty()) {
      expiry = GetRemainingExpiryTime(key, *protected_cache_);
      if (expiry > 0) {
        return true;
      }
      value = nullptr;
    }
  }

  if (mutable_cache_) {
    expiry = GetRemainingExpiryTime(key, *mutable_cache_);

    if (expiry > 0 || protected_keys_.IsProtected(key)) {
      // Entry didn't expire yet, we can still use it
      if (!PromoteKeyLru(key)) {
        // If not found in LRU or not protected no need to look in disk cache
        // either.
        OLP_SDK_LOG_DEBUG_F(kLogTag,
                            "Key not found in LRU, and not protected, key='%s'",
                            key.c_str());
        return false;
      }

      auto result = mutable_cache_->Get(key, value);
      return result && value;
    }

    // Data expired in cache -> remove, but not protected keys
    uint64_t removed_data_size = 0u;
    PurgeDiskItem(key, *mutable_cache_, removed_data_size);
    mutable_cache_data_size_ -= removed_data_size;
    RemoveKeyLru(key);
  }

  return false;
}

boost::optional<std::pair<std::string, time_t>>
DefaultCacheImpl::GetFromDiscCache(const std::string& key) {
  KeyValueCache::ValueTypePtr value = nullptr;
  time_t expiry = KeyValueCache::kDefaultExpiry;
  auto result = GetFromDiskCache(key, value, expiry);
  if (result && value) {
    return std::make_pair(std::string(value->begin(), value->end()), expiry);
  }

  return boost::none;
}

std::string DefaultCacheImpl::GetExpiryKey(const std::string& key) const {
  return CreateExpiryKey(key);
}

bool DefaultCacheImpl::Protect(const DefaultCache::KeyListType& keys) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  auto start = std::chrono::steady_clock::now();
  protected_keys_.Protect(keys, [&](const std::string& key) {
    if (!RemoveKeyLru(key)) {
      RemoveKeysWithPrefixLru(key);
    }
  });
  if (memory_cache_) {
    memory_cache_->Clear();
  }

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "Protect, time=%" PRId64 " ms, added keys size=%" PRIu64
                     ", total size=%" PRIu64,
                     GetElapsedTime(start),
                     static_cast<std::uint64_t>(keys.size()),
                     protected_keys_.Count());
  return true;
}

bool DefaultCacheImpl::Release(const DefaultCache::KeyListType& keys) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  auto start = std::chrono::steady_clock::now();
  auto result = protected_keys_.Release(keys);

  for (const auto& key : keys) {
    if (memory_cache_) {
      if (!memory_cache_->Remove(key)) {
        memory_cache_->RemoveKeysWithPrefix(key);
      }
    }
    if (mutable_cache_) {
      auto it = mutable_cache_->NewIterator(leveldb::ReadOptions());
      it->Seek(key);
      while (it->Valid()) {
        auto cached_key = it->key().ToString();
        if (cached_key.size() >= key.size() &&
            std::equal(key.begin(), key.end(), cached_key.begin())) {
          AddKeyLru(cached_key, it->value());
        } else {
          break;
        }
        it->Next();
      }
    }
  }

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "Release, time=%" PRId64 " ms, released keys size=%" PRIu64
                     ", total size=%" PRIu64,
                     GetElapsedTime(start),
                     static_cast<std::uint64_t>(keys.size()),
                     protected_keys_.Count());
  return result;
}

bool DefaultCacheImpl::IsProtected(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_lock_);
  return protected_keys_.IsProtected(key);
}

time_t DefaultCacheImpl::GetExpiryForMemoryCache(const std::string& key,
                                                 const time_t& expiry) {
  // reset expiry if key is protected for memory cache
  if (protected_keys_.IsProtected(key)) {
    return KeyValueCache::kDefaultExpiry;
  }
  return expiry;
}
}  // namespace cache
}  // namespace olp
