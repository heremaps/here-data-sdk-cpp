/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <atomic>
#include <ctime>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <leveldb/env.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/ApiResponse.h>

namespace leveldb {
class DB;
}  // namespace leveldb

namespace olp {
namespace cache {
class SizeCountingEnv;

/**
 * @brief The OpenResult enum defines the result of the open() operation.
 */
enum class OpenResult {
  /// Opening the store failed. Use openError() for details.
  Fail,
  /// The store was corrupted or store compaction was interrupted.
  Corrupted,
  /// The store was corrupted and has been repaired. Internal integrity might be
  /// broken.
  Repaired,
  /// The store was successfully opened.
  Success
};

struct StorageSettings {
  /// The maximum allowed size of storage on disk in bytes.
  uint64_t max_disk_storage = 0u;

  /// The maximum size of data in memory before it gets flushed to disk.
  /// Data is kept in memory until its size reaches this value and then data is
  /// flushed to disk. A maximum write buffer of 32 MB is most optimal even for
  /// batch imports.
  uint64_t max_chunk_size = 32 * 1024u * 1024u;

  /// Flag to enable double-writes to disk to avoid data losses between ignition
  /// cycles.
  bool enforce_immediate_flush = true;

  /// Maximum size of one file in storage, default 2MBytes.
  size_t max_file_size = 2 * 1024u * 1024u;

  /// Compression type to be applied on the data before storing it.
  leveldb::CompressionType compression =
      leveldb::CompressionType::kSnappyCompression;
};

/**
 * @brief Abstracts the disk database engine.
 */
class DiskCache {
 public:
  static constexpr uint64_t kSizeMax = std::numeric_limits<uint64_t>::max();

  /// No error type
  using NoError = client::ApiNoResult;

  /// Operation result type
  using OperationOutcome = client::ApiResponse<NoError, client::ApiError>;

  /// Will be used to filter out keys to be removed in case they are protected.
  using RemoveFilterFunc = std::function<bool(const std::string&)>;

  /// Logger that forwards leveldb log messages to our logging framework.
  class LevelDBLogger : public leveldb::Logger {
    void Logv(const char* format, va_list ap) override;
  };

  DiskCache();
  ~DiskCache();
  OpenResult Open(const std::string& data_path,
                  const std::string& versioned_data_path,
                  StorageSettings settings, OpenOptions options,
                  bool repair_if_broken = true);

  void Close();
  bool Clear();

  /// This method is blocking and calls the underlying leveldb CompactRange()
  /// method which compacts the storage. In particular, deleted and overwritten
  /// versions are discarded, and the data is rearranged to reduce the cost of
  /// operations needed to access the data. In some cases this operation might
  /// take a very long time, so use with care.
  void Compact();

  OperationOutcome OpenError() const { return error_; }

  bool Put(const std::string& key, leveldb::Slice slice);

  /// @deprecated Please use Get(const std::string&,
  /// KeyValueCache::ValueTypePtr&) instead.
  boost::optional<std::string> Get(const std::string& key);

  bool Get(const std::string& key, KeyValueCache::ValueTypePtr& value);

  /// Remove single key/value from DB.
  bool Remove(const std::string& key, uint64_t& removed_data_size);

  /// Get a new leveldb cache iterator. Use options.fill_cache = false for bulk
  /// scans.
  std::unique_ptr<leveldb::Iterator> NewIterator(leveldb::ReadOptions options);

  /// Allow batch writing so that we can delete and write multiple values at
  /// the same time.
  OperationOutcome ApplyBatch(std::unique_ptr<leveldb::WriteBatch> batch);

  /// Empty prefix deleted everything from DB. Returns size of removed data.
  bool RemoveKeysWithPrefix(const std::string& prefix,
                            uint64_t& removed_data_size,
                            const RemoveFilterFunc& filter = nullptr);

  /// Check if cache contains data with the key.
  bool Contains(const std::string& key);

  /// Gets size of the database: approximate for read-write, more-or-less
  /// precise for read-only
  uint64_t Size() const;

 private:
  /// Initialize empty db, so it can be used as protected cache.
  leveldb::Status InitializeDB(const StorageSettings& settings,
                               const std::string& path) const;

  /// Create options for DB basing on settings and cache type.
  leveldb::Options CreateOpenOptions(const StorageSettings& settings,
                                     bool is_read_only) const;

  std::string disk_cache_path_;
  std::unique_ptr<leveldb::DB> database_;
  std::unique_ptr<const leveldb::FilterPolicy> filter_policy_;
  std::unique_ptr<SizeCountingEnv> environment_;
  std::unique_ptr<LevelDBLogger> leveldb_logger_;
  uint64_t max_size_{kSizeMax};
  bool check_crc_{false};
  bool enforce_immediate_flush_{false};
  /// Used to sync database_->CompactRange() calls.
  std::atomic<bool> compacting_{false};
  /// Used to asynchronously call database_->CompactRange().
  std::thread compaction_thread_;
  OperationOutcome error_;
};

}  // namespace cache
}  // namespace olp
