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

#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include "DiskCacheSizeLimit.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/utils/Dir.h"

#define LOG_TAG "Storage.LevelDB"

namespace olp {
namespace cache {

namespace {
static inline leveldb::Slice toLeveldbSlice(const std::string& slice) {
  return leveldb::Slice(slice);
}

std::vector<std::string> tokenizePath(const std::string& dirName,
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
void createDir(const std::string& dirName) {
  auto dirNames = tokenizePath(dirName, "/");
  if (dirNames.size() <= 1u) {
    // For Windows path format - size less or equal 1 means path was'nt separate
    // by ('/') 4 * "\\\\" means "\" in path - 2x for string formatting purpose
    // 2x for regex.
    dirNames = tokenizePath(dirName, "\\\\");
  }
  std::string path = "/";
  for (const auto& dirName : dirNames) {
    path += dirName + "/";
    leveldb::Env::Default()->CreateDir(path);
  }
}

static bool repairCache(const std::string& versionedDataPath) {
  // first
  leveldb::Status status =
      leveldb::RepairDB(versionedDataPath, leveldb::Options());
  if (status.ok()) {
    EDGE_SDK_LOG_WARNING(LOG_TAG,
                         "Database corrupted, repaired " << versionedDataPath);
    leveldb::Env::Default()->DeleteDir(versionedDataPath + "/lost");
    return true;
  }
  EDGE_SDK_LOG_ERROR(
      LOG_TAG, "Database corrupted, repair failed: " << status.ToString());

  // repair failed, delete the entire cache;
  status = leveldb::DestroyDB(versionedDataPath, leveldb::Options());
  if (!status.ok()) {
    EDGE_SDK_LOG_ERROR(LOG_TAG, "Destroying database after corruption failed: "
                                    << status.ToString());
    return false;
  }
  EDGE_SDK_LOG_WARNING(
      LOG_TAG, "Destroyed database after corruption: " << versionedDataPath);
  return true;
}

void removeOtherDB(const std::string& dataPath,
                   const std::string& versionedDataPathToKeep) {
  std::vector<std::string> pathContents;
  leveldb::Status status =
      leveldb::Env::Default()->GetChildren(dataPath, &pathContents);
  if (!status.ok()) {
    EDGE_SDK_LOG_WARNING(
        LOG_TAG, "Clearing other DBs in folder: failed to list folder \""
                     << dataPath << "\" contents - " << status.ToString());
    return;
  }

  for (auto& item : pathContents) {
    // We shouldn't be checking .. for a database as we may not have rights and
    // should not be deleting files outside the specified folder
    if (item.compare("..") == 0) continue;

    const std::string fullPath = dataPath + '/' + item;
    if (fullPath != versionedDataPathToKeep) {
      status = leveldb::DestroyDB(fullPath, leveldb::Options());
      if (!status.ok())
        EDGE_SDK_LOG_WARNING(
            LOG_TAG,
            "Clearing other DBs in folder: failed to destroy database \""
                << fullPath << "\" - " << status.ToString());
    }
  }
}

}  // anonymous namespace

DiskCache::DiskCache() {}

DiskCache::~DiskCache() {
  // if (m_compactionThread.joinable()) m_compactionThread.join();

  Close();
}

void DiskCache::LevelDBLogger::Logv(const char* format, va_list ap) {
  if (!logging::Log::isEnabled(logging::Level::Trace,
                               "Storage.LevelDB.leveldb"))
    return;

  const std::string& message = logging::formatv(format, ap);
  logging::Log::logMessage(logging::Level::Trace, "Storage.LevelDB.leveldb",
                           message, __FILE__, __LINE__, __FUNCTION__,
                           EDGE_SDK_LOG_FUNCTION_SIGNATURE);
}

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

  leveldb::Options options;
  options.info_log = leveldb_logger_.get();

  // maximum write buffer of 32 MB is most optimal even for batch imports
  options.write_buffer_size = settings.max_chunk_size;
  if (settings.max_file_size != 0) {
    options.max_file_size = settings.max_file_size;
  }

  if (!isReadOnly) {
    options.create_if_missing = true;

    // create the directory if it doesn't exist
    createDir(dataPath);

    // remove other DBs only if provided the versioned path - do nothing
    // otherwise
    if (dataPath != versionedDataPath)
      removeOtherDB(dataPath, versionedDataPath);

    if (max_size_ != std::uint64_t(-1)) {
      environment_ = std::make_unique<DiskCacheSizeLimit>(
          leveldb::Env::Default(), versionedDataPath,
          settings.enforce_immediate_flush);
      options.env = environment_.get();
    }
  }

  leveldb::DB* db = nullptr;
  check_crc_ = openOptions & CheckCrc;

  // first attempt in opening the db
  leveldb::Status status = leveldb::DB::Open(options, versionedDataPath, &db);

  if (!status.ok() && !isReadOnly)
    EDGE_SDK_LOG_WARNING(LOG_TAG, "Cannot open database ("
                                      << status.ToString()
                                      << ") attempting repair");

  // if the database is r/w and corrupted, attempt to repair & reopen
  if ((status.IsCorruption() || status.IsIOError()) && !isReadOnly &&
      repairCache(versionedDataPath) == true) {
    status = leveldb::DB::Open(options, versionedDataPath, &db);
    if (status.ok()) {
      database_.reset(db);
      return OpenResult::Repaired;
    }
  }

  if (!status.ok()) {
    setOpenError(status);
    return OpenResult::Fail;
  }

  database_.reset(db);
  return OpenResult::Success;
}

void DiskCache::setOpenError(const leveldb::Status& status) {
  ErrorCode code = ErrorCode::Unknown;
  if (status.IsNotFound()) code = ErrorCode::NotFound;
  if (status.IsInvalidArgument()) code = ErrorCode::InvalidArgument;
  if (status.IsCorruption() || status.IsIOError())
    code = ErrorCode::InternalFailure;
  if (status.IsNotSupportedError()) code = ErrorCode::BadRequest;
  std::string errorMessage = status.ToString();
  EDGE_SDK_LOG_FATAL(LOG_TAG, "Cannot open database " << errorMessage);
  error_ = ApiError(code, std::move(errorMessage));
}

bool DiskCache::Put(const std::string& key, const std::string& value) {
  if (!database_) return false;

  if (environment_ && (max_size_ != std::uint64_t(-1))) {
    if (environment_->size() >= max_size_) return false;
  }

  const leveldb::Status& status = database_->Put(
      leveldb::WriteOptions(), toLeveldbSlice(key), toLeveldbSlice(value));
  if (!status.ok()) {
    EDGE_SDK_LOG_FATAL(LOG_TAG,
                       "Failed to write the database " << status.ToString());
    return false;
  }
  return true;
}

boost::optional<std::string> DiskCache::Get(const std::string& key) {
  if (!database_) return boost::none;
  std::string res;
  return database_->Get({}, toLeveldbSlice(key), &res).ok()
             ? boost::make_optional(res)
             : boost::none;
}

bool DiskCache::Remove(const std::string& key) {
  if (!database_ || !database_->Delete(leveldb::WriteOptions(), key).ok())
    return false;
  return true;
}

bool DiskCache::RemoveKeysWithPrefix(const std::string& keyPrefix) {
  if (!database_) return false;
  auto batch = std::make_unique<leveldb::WriteBatch>();

  leveldb::ReadOptions opts;
  opts.verify_checksums = check_crc_;
  opts.fill_cache = true;
  std::unique_ptr<leveldb::Iterator> iterator;
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

  const leveldb::Status& status =
      database_->Write(leveldb::WriteOptions(), batch.get());
  if (!status.ok()) {
    EDGE_SDK_LOG_FATAL(LOG_TAG,
                       "Failed to write the database " << status.ToString();
                       return false;);
  }
  return true;
}

}  // namespace cache
}  // namespace olp
