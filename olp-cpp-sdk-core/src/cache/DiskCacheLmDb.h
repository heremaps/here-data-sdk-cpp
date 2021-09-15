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

#include <leveldb/options.h>  // For leveldb::CompressionType used in StorageSettings
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>

#include "cache/lmdb.h"  // TODO: link statically in a correct way, move to cpp

// TODO: Think about how to make forward declaration of MDB_cursor_op
// to fix compilation error on gcc

/*struct MDB_cursor;
struct MDB_val;
enum MDB_cursor_op;
struct MDB_env;
typedef unsigned int MDB_dbi;*/

namespace olp {
namespace cache {

// Wrapper for lmdb cursor
class CursorWrapper {
 public:
  CursorWrapper(MDB_cursor* cursor);
  ~CursorWrapper();

  CursorWrapper(const CursorWrapper&) = delete;
  CursorWrapper(const CursorWrapper&&) = delete;
  CursorWrapper& operator=(const CursorWrapper&) = delete;
  CursorWrapper& operator=(const CursorWrapper&&) = delete;

  int Get(MDB_val* key, MDB_val* value, MDB_cursor_op option) const;

  int Put(MDB_val* key, MDB_val* value, unsigned int flags);

  int Put(const std::string& key, const std::string& value,
          unsigned int flags = 0);

  int Put(const std::string& key, const KeyValueCache::ValueTypePtr value,
          unsigned int flags = 0);

  int Del(MDB_val* key, unsigned int flags);

  int Del(const std::string& key, unsigned int flags = 0);

  bool IsValid() const;

  int CommitTransaction();

  void AbortTransaction();

  int GetLastResult() const;

  MDB_cursor* GetCursor();

 private:
  MDB_cursor* cursor_;
  mutable int last_result_{0};
};

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
class DiskCacheLmDb {
 public:
  static constexpr uint64_t kSizeMax = std::numeric_limits<uint64_t>::max();

  /// Will be used to filter out keys to be removed in case they are protected.
  using RemoveFilterFunc = std::function<bool(const std::string&)>;

  DiskCacheLmDb();
  ~DiskCacheLmDb();
  OpenResult Open(const std::string& data_path, StorageSettings settings,
                  OpenOptions options);  // repair_if_broken is not used in lmdb

  void Close();
  bool Clear();

  void Compact();  // Compaction is not needed in lmdb

  /// @deprecated Please use Get(const std::string&,
  /// KeyValueCache::ValueTypePtr&) instead.
  boost::optional<std::string> Get(const std::string& key) const;

  bool Get(const std::string& key, KeyValueCache::ValueTypePtr& value) const;

  /// Remove single key/value from DB.
  bool Remove(const std::string& key, uint64_t& removed_data_size);

  /// Get a new lmdb cursor.
  MDB_cursor* NewCursor(bool read_only) const;

  /// Empty prefix deleted everything from DB. Returns size of removed data.
  bool RemoveKeysWithPrefix(const std::string& prefix,
                            uint64_t& removed_data_size,
                            const RemoveFilterFunc& filter = nullptr);

  /// Check if cache contains data with the key.
  bool Contains(const std::string& key) const;

  /// Gets size of the database: approximate for read-write, more-or-less
  /// precise for read-only
  uint64_t Size() const;

 private:
  int InitDataBaseHandle(MDB_dbi* database_handle, bool read_only) const;

  MDB_cursor* SetCursorRange(const std::string& prefix, MDB_val* mkey,
                             MDB_val* mvalue) const;

  MDB_env* lmdb_env_{nullptr};
  MDB_dbi database_handle_{0u};
  bool is_read_only_{false};  // allow only read operations
  std::string disk_cache_path_;
  uint64_t max_size_{kSizeMax};
};

}  // namespace cache
}  // namespace olp
