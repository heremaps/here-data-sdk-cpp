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

#include "olp/core/cache/DefaultCache.h"
#include "DiskCache.h"
#include "InMemoryCache.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"

#define LOGTAG "DefaultCache"

namespace {
std::string CreateExpiryKey(const std::string& key) { return key + "::expiry"; }

time_t GetRemainingExpiryTime(const std::string& key,
                              olp::cache::DiskCache& disk_cache) {
  auto expiry_key = CreateExpiryKey(key);
  auto expiry = (std::numeric_limits<time_t>::max)();
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

DefaultCache::DefaultCache(const CacheSettings& settings)
    : settings_(settings),
      is_open_(false),
      memory_cache_(nullptr),
      disk_cache_(nullptr) {}

DefaultCache::~DefaultCache() {}

DefaultCache::StorageOpenResult DefaultCache::Open() {
  std::lock_guard<std::mutex> lk(cache_lock_);
  is_open_ = true;
  return SetupStorage();
}

void DefaultCache::Close() {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return;
  }
  if (memory_cache_) memory_cache_.reset();
  if (disk_cache_) disk_cache_.reset();
  is_open_ = false;
}

bool DefaultCache::Clear() {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    memory_cache_->Clear();
  }

  if (disk_cache_) {
    if (!disk_cache_->Clear()) {
      return false;
    }
  }
  if (SetupStorage() != DefaultCache::StorageOpenResult::Success) {
    EDGE_SDK_LOG_DEBUG_F(LOGTAG, "Failed to reopen the diskcache %s",
                         settings_.disk_path.get().c_str());
    return false;
  }
  return true;
}

bool DefaultCache::Put(const std::string& key, const boost::any& value,
                       const Encoder& encoder, time_t expiry) {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return false;
  }
  auto encodedItem = encoder();
  if (memory_cache_) {
    if (!memory_cache_->Put(key, value, expiry, encodedItem.size()))
      return false;
  }

  if (disk_cache_) {
    if (expiry < (std::numeric_limits<time_t>::max)()) {
      if (!StoreExpiry(key, *disk_cache_, expiry)) {
        return false;
      }
    }

    if (!disk_cache_->Put(key, encodedItem)) {
      return false;
    }
  }
  return true;
}

bool DefaultCache::Put(const std::string& key,
                       const std::shared_ptr<std::vector<unsigned char>> value,
                       time_t expiry) {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    if (!memory_cache_->Put(key, value, expiry, value->size())) {
      return false;
    }
  }

  if (disk_cache_) {
    if (expiry < (std::numeric_limits<time_t>::max)()) {
      if (!StoreExpiry(key, *disk_cache_, expiry)) {
        return false;
      }
    }

    std::string val(value->begin(), value->end());
    if (!disk_cache_->Put(key, val)) {
      return false;
    }
  }

  return true;
}

boost::any DefaultCache::Get(const std::string& key, const Decoder& decoder) {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return boost::any();
  }

  if (memory_cache_) {
    auto value = memory_cache_->Get(key);
    if (!value.empty()) {
      return value;
    }
  }

  if (disk_cache_) {
    auto expiry = GetRemainingExpiryTime(key, *disk_cache_);
    if (expiry <= 0) {
      PurgeDiskItem(key, *disk_cache_);
      return boost::any();
    }

    auto value = disk_cache_->Get(key);
    if (value) {
      auto decoded_item = decoder(value.get());
      if (memory_cache_) {
        memory_cache_->Put(key, decoded_item, expiry, value->size());
      }
      return decoded_item;
    }
  }

  return boost::any();
}

std::shared_ptr<std::vector<unsigned char>> DefaultCache::Get(
    const std::string& key) {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return nullptr;
  }

  if (memory_cache_) {
    auto value = memory_cache_->Get(key);

    if (!value.empty()) {
      return boost::any_cast<std::shared_ptr<std::vector<unsigned char>>>(
          value);
    }
  }

  if (disk_cache_) {
    auto expiry = GetRemainingExpiryTime(key, *disk_cache_);
    if (expiry <= 0) {
      PurgeDiskItem(key, *disk_cache_);
      return nullptr;
    }

    auto value = disk_cache_->Get(key);
    if (value) {
      auto data = std::make_shared<std::vector<unsigned char>>(
          value.get().begin(), value.get().end());
      if (memory_cache_) {
        memory_cache_->Put(key, data, expiry, value->size());
      }
      return data;
    }
  }

  return nullptr;
}

bool DefaultCache::Remove(const std::string& key) {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    memory_cache_->Remove(key);
  }

  if (disk_cache_) {
    if (!disk_cache_->Remove(key)) return false;
  }

  return true;
}

bool DefaultCache::RemoveKeysWithPrefix(const std::string& key) {
  std::lock_guard<std::mutex> lk(cache_lock_);
  if (!is_open_) {
    return false;
  }

  if (memory_cache_) {
    memory_cache_->RemoveKeysWithPrefix(key);
  }

  if (disk_cache_) {
    return disk_cache_->RemoveKeysWithPrefix(key);
  }
  return true;
}  // namespace cache

DefaultCache::StorageOpenResult DefaultCache::SetupStorage() {
  auto result = Success;
  if (memory_cache_) {
    memory_cache_.reset();
  }

  if (disk_cache_) {
    disk_cache_.reset();
  }
  if (settings_.max_memory_cache_size > 0) {
    memory_cache_.reset(new InMemoryCache(settings_.max_memory_cache_size));
  }
  if (settings_.disk_path) {
    StorageSettings storage_settings;
    storage_settings.max_disk_storage = settings_.max_disk_storage;
    storage_settings.max_chunk_size = settings_.max_chunk_size;
    storage_settings.enforce_immediate_flush =
        settings_.enforce_immediate_flush;
    storage_settings.max_file_size = settings_.max_file_size;

    disk_cache_ = std::make_unique<DiskCache>();
    auto status =
        disk_cache_->Open(settings_.disk_path.get(), settings_.disk_path.get(),
                          storage_settings, OpenOptions::Default);
    if (status == OpenResult::Fail) {
      disk_cache_.reset();
      settings_.disk_path = boost::none;
      result = OpenDiskPathFailure;
    }
  }

  return result;
}

}  // namespace cache

}  // namespace olp
