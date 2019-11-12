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
#include <vector>

#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include "DiskCacheSizeLimitEnv.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/utils/Dir.h"

namespace olp {
namespace cache {

namespace {

constexpr auto kLogTag = "Storage.RocksDB";

static inline rocksdb::Slice ToSlice(const std::string& slice) {
  return rocksdb::Slice(slice);
}

std::vector<std::string> TokenizePath(const std::string& dirName,
                                      std::string delimiter) {
  std::regex regexp{"[" + delimiter + "]+"};
  std::sregex_token_iterator iterator(dirName.begin(), dirName.end(), regexp,
                                      -1);
  std::vector<std::string> dirNames;
  std::copy_if(iterator, {}, std::back_inserter(dirNames),
               [](const std::string& matched_element) {
                 return !matched_element.empty();
               });
  return dirNames;
}

// Create all nested directories according to the provided path ('dirName').
void CreateDir(const std::string& dirName) {
  auto dirNames = TokenizePath(dirName, "/");
  if (dirNames.size() <= 1u) {
    // For Windows path format - size less or equal 1 means path was'nt separate
    // by ('/') 4 * "\\\\" means "\" in path - 2x for string formatting purpose
    // 2x for regex.
    dirNames = TokenizePath(dirName, "\\\\");
  }
  std::string path = "/";
  for (const auto& dirName : dirNames) {
    path += dirName + "/";
    rocksdb::Env::Default()->CreateDir(path);
  }
}

bool RepairCache(const std::string& versionedDataPath) {
  // first
  rocksdb::Status status =
      rocksdb::RepairDB(versionedDataPath, rocksdb::Options());
  if (status.ok()) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "Database corrupted, repaired " << versionedDataPath);
    rocksdb::Env::Default()->DeleteDir(versionedDataPath + "/lost");
    return true;
  }
  OLP_SDK_LOG_ERROR(kLogTag,
                    "Database corrupted, repair failed: " << status.ToString());

  // repair failed, delete the entire cache;
  status = rocksdb::DestroyDB(versionedDataPath, rocksdb::Options());
  if (!status.ok()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Destroying database after corruption failed: "
                                   << status.ToString());
    return false;
  }
  OLP_SDK_LOG_WARNING(
      kLogTag, "Destroyed database after corruption: " << versionedDataPath);
  return true;
}

void RemoveOtherDB(const std::string& dataPath,
                   const std::string& versionedDataPathToKeep) {
  std::vector<std::string> pathContents;
  rocksdb::Status status =
      rocksdb::Env::Default()->GetChildren(dataPath, &pathContents);
  if (!status.ok()) {
    OLP_SDK_LOG_WARNING(
        kLogTag, "Clearing other DBs in folder: failed to list folder \""
                     << dataPath << "\" contents - " << status.ToString());
    return;
  }

  for (auto& item : pathContents) {
    // We shouldn't be checking .. for a database as we may not have rights and
    // should not be deleting files outside the specified folder
    if (item.compare("..") == 0) continue;

    const std::string fullPath = dataPath + '/' + item;
    if (fullPath != versionedDataPathToKeep) {
      status = rocksdb::DestroyDB(fullPath, rocksdb::Options());
      if (!status.ok())
        OLP_SDK_LOG_WARNING(
            kLogTag,
            "Clearing other DBs in folder: failed to destroy database \""
                << fullPath << "\" - " << status.ToString());
    }
  }
}

// logger that forwards leveldb log messages to our logging framework
class LevelDBLogger : public rocksdb::Logger {
 public:
  void Logv(const char* format, va_list ap) override {
    std::string message = logging::formatv(format, ap);
    OLP_SDK_LOG(level_, kLogTag, message);
  }

  void SetInfoLogLevel(const rocksdb::InfoLogLevel log_level) override {
    rocksdb::Logger::SetInfoLogLevel(log_level);
    switch (log_level) {
      case rocksdb::InfoLogLevel::DEBUG_LEVEL:
        level_ = logging::Level::Debug;
        break;
      case rocksdb::InfoLogLevel::INFO_LEVEL:
      case rocksdb::InfoLogLevel::HEADER_LEVEL:
        level_ = logging::Level::Info;
        break;
      case rocksdb::InfoLogLevel::WARN_LEVEL:
        level_ = logging::Level::Warning;
        break;
      case rocksdb::InfoLogLevel::ERROR_LEVEL:
        level_ = logging::Level::Error;
        break;
      case rocksdb::InfoLogLevel::FATAL_LEVEL:
        level_ = logging::Level::Fatal;
        break;
    }
  }

 private:
  logging::Level level_ = logging::Level::Debug;
};

ApiError StatusToApiError(const rocksdb::Status& status) {
  ErrorCode code = ErrorCode::Unknown;
  if (status.IsNotFound()) code = ErrorCode::NotFound;
  if (status.IsInvalidArgument()) code = ErrorCode::InvalidArgument;
  if (status.IsCorruption() || status.IsIOError())
    code = ErrorCode::InternalFailure;
  if (status.IsNotSupported()) code = ErrorCode::BadRequest;
  std::string errorMessage = status.ToString();
  OLP_SDK_LOG_FATAL(kLogTag, "Cannot open database " << errorMessage);
  return ApiError(code, std::move(errorMessage));
}

}  // anonymous namespace

DiskCache::DiskCache() {}

DiskCache::~DiskCache() { Close(); }

void DiskCache::Close() { database_.reset(); }
bool DiskCache::Clear() {
  database_.reset();
  if (!disk_cache_path_.empty()) {
    return olp::utils::Dir::remove(disk_cache_path_);
  }
  return true;
}

OpenResult DiskCache::Open(const std::string& dataPath,
                           const std::string& versionedDataPath,
                           StorageSettings settings, OpenOptions openOptions) {
  disk_cache_path_ = dataPath;
  if (!olp::utils::Dir::exists(disk_cache_path_)) {
    if (!olp::utils::Dir::create(disk_cache_path_)) {
      return OpenResult::Fail;
    }
  }

  bool isReadOnly = (openOptions & ReadOnly) == ReadOnly;

  max_size_ = settings.max_disk_storage;

  rocksdb::Options options;
  options.OptimizeForSmallDb();
  options.info_log = std::make_shared<LevelDBLogger>();

  logging::Log::setLevel(logging::Level::Warning, "Storage.LevelDB.leveldb");

  // maximum write buffer of 32 MB is most optimal even for batch imports
  options.write_buffer_size = settings.max_chunk_size;
  if (settings.max_file_size != 0) {
    // options.max_file_size = settings.max_file_size;
  }

  if (!isReadOnly) {
    options.create_if_missing = true;

    // create the directory if it doesn't exist
    CreateDir(dataPath);

    // remove other DBs only if provided the versioned path - do nothing
    // otherwise
    if (dataPath != versionedDataPath)
      RemoveOtherDB(dataPath, versionedDataPath);

    if (max_size_ != std::uint64_t(-1)) {
      environment_ = std::make_unique<DiskCacheSizeLimitEnv>(
          rocksdb::Env::Default(), versionedDataPath,
          settings.enforce_immediate_flush);
      options.env = environment_.get();
    }
  }

  rocksdb::DB* db = nullptr;
  check_crc_ = openOptions & CheckCrc;

  // first attempt in opening the db
  rocksdb::Status status = rocksdb::DB::Open(options, versionedDataPath, &db);

  if (!status.ok() && !isReadOnly)
    OLP_SDK_LOG_WARNING(kLogTag, "Cannot open database ("
                                     << status.ToString()
                                     << ") attempting repair");

  // if the database is r/w and corrupted, attempt to repair & reopen
  if ((status.IsCorruption() || status.IsIOError()) && !isReadOnly &&
      RepairCache(versionedDataPath) == true) {
    status = rocksdb::DB::Open(options, versionedDataPath, &db);
    if (status.ok()) {
      database_.reset(db);
      return OpenResult::Repaired;
    }
  }

  if (!status.ok()) {
    error_ = StatusToApiError(status);
    return OpenResult::Fail;
  }

  database_.reset(db);
  return OpenResult::Success;
}

bool DiskCache::Put(const std::string& key, const std::string& value) {
  if (!database_) return false;

  if (environment_ && (max_size_ != std::uint64_t(-1))) {
    if (environment_->Size() >= max_size_) return false;
  }

  const rocksdb::Status& status =
      database_->Put(rocksdb::WriteOptions(), ToSlice(key), ToSlice(value));
  if (!status.ok()) {
    OLP_SDK_LOG_FATAL(kLogTag,
                      "Failed to write the database " << status.ToString());
    return false;
  }
  return true;
}

boost::optional<std::string> DiskCache::Get(const std::string& key) {
  if (!database_) return boost::none;
  std::string res;
  return database_->Get({}, ToSlice(key), &res).ok() ? boost::make_optional(res)
                                                     : boost::none;
}

bool DiskCache::Remove(const std::string& key) {
  if (!database_ || !database_->Delete(rocksdb::WriteOptions(), key).ok())
    return false;
  return true;
}

bool DiskCache::RemoveKeysWithPrefix(const std::string& keyPrefix) {
  if (!database_) return false;
  auto batch = std::make_unique<rocksdb::WriteBatch>();

  rocksdb::ReadOptions opts;
  opts.verify_checksums = check_crc_;
  opts.fill_cache = true;
  std::unique_ptr<rocksdb::Iterator> iterator;
  iterator.reset(database_->NewIterator(opts));

  if (keyPrefix.empty()) {
    for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      batch->Delete(iterator->key());
    }
  } else {
    for (iterator->Seek(keyPrefix);
         iterator->Valid() && iterator->key().starts_with(keyPrefix);
         iterator->Next()) {
      batch->Delete(iterator->key());
    }
  }

  const rocksdb::Status& status =
      database_->Write(rocksdb::WriteOptions(), batch.get());
  if (!status.ok()) {
    OLP_SDK_LOG_FATAL(kLogTag,
                      "Failed to write the database " << status.ToString();
                      return false;);
  }
  return true;
}

}  // namespace cache
}  // namespace olp
