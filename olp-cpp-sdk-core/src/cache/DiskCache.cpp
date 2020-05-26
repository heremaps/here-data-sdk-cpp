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

#include "DiskCache.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include "DiskCacheSizeLimitEnv.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/utils/Dir.h"

namespace olp {
namespace cache {

namespace {
constexpr auto kLogTag = "Storage.LevelDB";

leveldb::Slice ToLeveldbSlice(const std::string& slice) {
  return leveldb::Slice(slice);
}

std::vector<std::string> TokenizePath(const std::string& directory_name,
                                      std::string delimiter) {
  std::regex regexp{"[" + delimiter + "]+"};
  std::sregex_token_iterator iterator(directory_name.begin(),
                                      directory_name.end(), regexp, -1);
  std::vector<std::string> directory_names;
  std::copy_if(iterator, {}, std::back_inserter(directory_names),
               [](const std::string& matched_element) {
                 return !matched_element.empty();
               });
  return directory_names;
}

// Create all nested directories according to the provided path ('dirName').
void CreateDir(const std::string& directory_name) {
  auto directory_names = TokenizePath(directory_name, "/");
  if (directory_names.size() <= 1u) {
    // For Windows path format - size less or equal 1 means path wasn't separate
    // by ('/') 4 * "\\\\" means "\" in path - 2x for string formatting purpose
    // 2x for regex.
    directory_names = TokenizePath(directory_name, "\\\\");
  }

  std::string path = "/";
  for (const auto& name : directory_names) {
    path += name + "/";
    leveldb::Env::Default()->CreateDir(path);
  }
}

static bool RepairCache(const std::string& data_path) {
  // first
  auto status = leveldb::RepairDB(data_path, leveldb::Options());
  if (status.ok()) {
    OLP_SDK_LOG_INFO(kLogTag, "RepairCache: repaired - " << data_path);
    leveldb::Env::Default()->DeleteDir(data_path + "/lost");
    return true;
  }
  OLP_SDK_LOG_ERROR(kLogTag,
                    "RepairCache: repair failed - " << status.ToString());

  // repair failed, delete the entire cache;
  status = leveldb::DestroyDB(data_path, leveldb::Options());
  if (!status.ok()) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "RepairCache: destroying corrupted database failed - "
                          << status.ToString());
    return false;
  }
  OLP_SDK_LOG_WARNING(
      kLogTag, "RepairCache: destroyed corrupted database - " << data_path);
  return true;
}

void RemoveOtherDB(const std::string& data_path,
                   const std::string& data_path_to_keep) {
  std::vector<std::string> path_contents;
  auto status = leveldb::Env::Default()->GetChildren(data_path, &path_contents);
  if (!status.ok()) {
    OLP_SDK_LOG_WARNING(kLogTag, "RemoveOtherDB: failed to list folder \""
                                     << data_path << "\" contents - "
                                     << status.ToString());
    return;
  }

  for (auto& item : path_contents) {
    // We shouldn't be checking .. for a database as we may not have rights and
    // should not be deleting files outside the specified folder
    if (item.compare("..") == 0) {
      continue;
    }

    const std::string full_path = data_path + '/' + item;
    if (full_path == data_path_to_keep) {
      continue;
    }

    status = leveldb::DestroyDB(full_path, leveldb::Options());
    if (!status.ok()) {
      OLP_SDK_LOG_WARNING(kLogTag,
                          "RemoveOtherDB: failed to destroy database \""
                              << full_path << "\" - " << status.ToString());
    }
  }
}

client::ApiError GetApiError(const leveldb::Status& status) {
  client::ErrorCode code = client::ErrorCode::Unknown;
  if (status.IsNotFound()) {
    code = client::ErrorCode::NotFound;
  }
  if (status.IsInvalidArgument()) {
    code = client::ErrorCode::InvalidArgument;
  }
  if (status.IsCorruption() || status.IsIOError()) {
    code = client::ErrorCode::InternalFailure;
  }
  if (status.IsNotSupportedError()) {
    code = client::ErrorCode::BadRequest;
  }
  return client::ApiError(code, status.ToString());
}

}  // anonymous namespace

DiskCache::DiskCache() = default;

DiskCache::~DiskCache() { Close(); }

void DiskCache::LevelDBLogger::Logv(const char* format, va_list ap) {
  OLP_SDK_LOG_DEBUG_F("Storage.LevelDB.leveldb", format, ap);
}

void DiskCache::Close() {
  database_.reset();
  filter_policy_.reset();
}

bool DiskCache::Clear() {
  database_.reset();
  filter_policy_.reset();
  if (!disk_cache_path_.empty()) {
    return olp::utils::Dir::remove(disk_cache_path_);
  }
  return true;
}

OpenResult DiskCache::Open(const std::string& data_path,
                           const std::string& versioned_data_path,
                           StorageSettings settings, OpenOptions options) {
  disk_cache_path_ = data_path;
  if (!olp::utils::Dir::exists(disk_cache_path_)) {
    if (!olp::utils::Dir::create(disk_cache_path_)) {
      return OpenResult::Fail;
    }
  }

  bool is_read_only = (options & ReadOnly) == ReadOnly;
  max_size_ = settings.max_disk_storage;

  leveldb::Options open_options;
  open_options.compression = settings.compression;
  open_options.info_log = leveldb_logger_.get();
  open_options.write_buffer_size = settings.max_chunk_size;
  open_options.filter_policy = leveldb::NewBloomFilterPolicy(10);
  filter_policy_.reset(open_options.filter_policy);
  if (settings.max_file_size != 0) {
    open_options.max_file_size = settings.max_file_size;
  }

  if (!is_read_only) {
    open_options.create_if_missing = true;

    // Create the directory if it doesn't exist
    CreateDir(data_path);

    // Remove other DBs only if provided the versioned path - do nothing
    // otherwise
    if (data_path != versioned_data_path)
      RemoveOtherDB(data_path, versioned_data_path);

    if (max_size_ != kSizeMax) {
      environment_ = std::make_unique<DiskCacheSizeLimitEnv>(
          leveldb::Env::Default(), versioned_data_path,
          settings.enforce_immediate_flush);
      open_options.env = environment_.get();
    }
  }

  leveldb::DB* db = nullptr;
  check_crc_ = options & CheckCrc;

  // First attempt in opening the db
  auto status = leveldb::DB::Open(open_options, versioned_data_path, &db);

  if (!status.ok() && !is_read_only)
    OLP_SDK_LOG_WARNING(kLogTag, "Open: failed, attempting repair, error="
                                     << status.ToString());

  // If the database is r/w and corrupted, attempt to repair & reopen
  if ((status.IsCorruption() || status.IsIOError()) && !is_read_only &&
      RepairCache(versioned_data_path) == true) {
    status = leveldb::DB::Open(open_options, versioned_data_path, &db);
    if (status.ok()) {
      database_.reset(db);
      return OpenResult::Repaired;
    }
  }

  if (!status.ok()) {
    error_ = GetApiError(status);
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Open: failed, error=" << error_.GetError().GetMessage());
    return OpenResult::Fail;
  } else {
    error_ = NoError{};
  }

  database_.reset(db);
  return OpenResult::Success;
}

bool DiskCache::Put(const std::string& key, leveldb::Slice slice) {
  if (!database_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Put: Database is uninitialized");
    return false;
  }

  const auto status =
      database_->Put(leveldb::WriteOptions(), ToLeveldbSlice(key), slice);
  if (!status.ok()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Put: failed, status=" << status.ToString());
    return false;
  }
  return true;
}

boost::optional<std::string> DiskCache::Get(const std::string& key) {
  if (!database_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Get: Database is uninitialized");
    return boost::none;
  }

  std::string res;
  leveldb::ReadOptions options;
  options.verify_checksums = check_crc_;
  return database_->Get(options, ToLeveldbSlice(key), &res).ok()
             ? boost::optional<std::string>(std::move(res))
             : boost::none;
}

bool DiskCache::Get(const std::string& key,
                    KeyValueCache::ValueTypePtr& value) {
  if (!database_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Get: Database not initialized");
    return false;
  }

  value = nullptr;
  leveldb::ReadOptions options;
  options.verify_checksums = check_crc_;
  auto iterator = NewIterator(options);

  iterator->Seek(key);
  if (iterator->Valid() && iterator->key() == key) {
    auto slice_value = iterator->value();
    if (!slice_value.empty()) {
      value = std::make_shared<KeyValueCache::ValueType>(
          slice_value.data(), slice_value.data() + slice_value.size());
    }
  }

  return true;
}

bool DiskCache::Remove(const std::string& key, uint64_t& removed_data_size) {
  if (!database_) {
    removed_data_size = 0;

    OLP_SDK_LOG_ERROR(kLogTag, "Remove: Database is uninitialized");
    return false;
  }

  uint64_t data_size = 0u;
  auto it = NewIterator({});
  it->Seek(key);
  if (it->Valid() && it->key() == key) {
    data_size = key.size() + it->value().size();
  }

  auto result = database_->Delete(leveldb::WriteOptions(), key).ok();
  if (result) {
    removed_data_size = data_size;
  }

  return result;
}

std::unique_ptr<leveldb::Iterator> DiskCache::NewIterator(
    leveldb::ReadOptions options) {
  if (!database_) {
    OLP_SDK_LOG_ERROR(kLogTag, "NewIterator: Database is uninitialized");
    return nullptr;
  }
  return std::unique_ptr<leveldb::Iterator>(database_->NewIterator(options));
}

DiskCache::OperationOutcome DiskCache::ApplyBatch(
    std::unique_ptr<leveldb::WriteBatch> batch) {
  if (!database_) {
    OLP_SDK_LOG_ERROR(kLogTag, "ApplyBatch: Database is uninitialized");
    return client::ApiError(client::ErrorCode::PreconditionFailed,
                            "Database is not initialized");
  }
  if (!batch) {
    OLP_SDK_LOG_ERROR(kLogTag, "ApplyBatch: failed, batch is null");
    return client::ApiError(client::ErrorCode::PreconditionFailed,
                            "Batch can't be null");
  }
  const auto status = database_->Write(leveldb::WriteOptions(), batch.get());
  if (!status.ok()) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "ApplyBatch: failed, status=" << status.ToString());
    return GetApiError(status);
  }
  return NoError{};
}

bool DiskCache::RemoveKeysWithPrefix(const std::string& prefix,
                                     uint64_t& removed_data_size) {
  uint64_t data_size = 0u;
  if (!database_) {
    removed_data_size = 0u;
    OLP_SDK_LOG_ERROR(kLogTag,
                      "RemoveKeysWithPrefix: Database is uninitialized");
    return false;
  }

  auto batch = std::make_unique<leveldb::WriteBatch>();

  leveldb::ReadOptions opts;
  opts.verify_checksums = check_crc_;
  opts.fill_cache = true;
  auto iterator = NewIterator(opts);

  if (prefix.empty()) {
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      batch->Delete(iterator->key());
      data_size += iterator->value().size() + iterator->key().size();
    }
  } else {
    for (iterator->Seek(prefix);
         iterator->Valid() && iterator->key().starts_with(prefix);
         iterator->Next()) {
      batch->Delete(iterator->key());
      data_size += iterator->value().size() + iterator->key().size();
    }
  }

  auto result = ApplyBatch(std::move(batch));
  if (!result.IsSuccessful()) {
    data_size = 0u;
  }

  removed_data_size = data_size;
  return result.IsSuccessful();
}

}  // namespace cache
}  // namespace olp
