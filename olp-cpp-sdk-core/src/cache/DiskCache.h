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

#pragma once

#include <time.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include <leveldb/env.h>
#include <leveldb/write_batch.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/ApiError.h>

namespace leveldb {
class DB;
}  // namespace leveldb

using namespace olp::client;

namespace olp {
namespace cache {

class DiskCacheSizeLimitEnv;

/**
 * @brief The OpenResult enum defines the result of the open() operation
 */
enum class OpenResult {
  /// Opening the store failed. Use openError() for details
  Fail,
  /// The store was corrupted and has been repaired. Internal integrity might be
  /// broken.
  Repaired,
  /// The store was successfully opened
  Success
};

struct StorageSettings {
  /// maxSizeB is maximum size of storage on disk in bytes
  uint64_t max_disk_storage = 0u;
  /// chunkMaxSizeB is maximum size of data in memory before it gets flushed
  /// to disk. Data is kept in memory until its size reaches this value and
  /// then data is flushed to disk.
  uint64_t max_chunk_size = 1024 * 1024 * 32;
  /// enforceImmediateFlush flag to enable double-writes to disk to avoid data
  /// losses between ignition cycles.
  bool enforce_immediate_flush = true;
  /// Maximum size of one file in storage, default 2mb
  size_t max_file_size = 2 * 1024u * 1024u;
};

class DiskCache {
 public:
  // logger that forwards leveldb log messages to our logging framework
  class LevelDBLogger : public leveldb::Logger {
    void Logv(const char* format, va_list ap) override;
  };

  DiskCache();
  ~DiskCache();
  OpenResult Open(const std::string& dataPath,
                  const std::string& versionedDataPath,
                  StorageSettings settings, OpenOptions openOptions);

  void Close();

  bool Clear();

  ApiError openError() const { return error_; }

  bool Put(const std::string& key, const std::string& value);

  boost::optional<std::string> Get(const std::string& key);

  size_t Size() const;

  bool Remove(const std::string& key);
  bool RemoveKeysWithPrefix(const std::string& keyPrefix);

 private:
  void setOpenError(const leveldb::Status& status);
  bool applyBatch(std::unique_ptr<leveldb::WriteBatch> batch);

 private:
  std::string disk_cache_path_;
  std::unique_ptr<leveldb::DB> database_;
  std::unique_ptr<DiskCacheSizeLimitEnv> environment_;
  std::unique_ptr<LevelDBLogger> leveldb_logger_;
  uint64_t max_size_{std::uint64_t(-1)};
  bool check_crc_{false};
  ApiError error_;
};

}  // namespace cache
}  // namespace olp
