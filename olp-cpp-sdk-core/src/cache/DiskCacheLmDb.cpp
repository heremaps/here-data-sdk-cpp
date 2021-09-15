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

#include "DiskCacheLmDb.h"

#include "olp/core/logging/Log.h"
#include "olp/core/utils/Dir.h"

namespace olp {
namespace cache {

// Wrapper for lmdb cursor
CursorWrapper::CursorWrapper(MDB_cursor* cursor) : cursor_(cursor) {}

CursorWrapper::~CursorWrapper() {
  if (!IsValid()) {
    return;
  }
  auto* txn = mdb_cursor_txn(cursor_);
  mdb_cursor_close(cursor_);
  if (txn == nullptr) {
    return;
  }
  if (last_result_ == 0) {
    if (const auto commit_result = mdb_txn_commit(txn)) {
      OLP_SDK_LOG_DEBUG_F("CursorWrapper",
                          "~CursorWrapper(): Transaction was not commited, "
                          "last_result=%d",
                          commit_result);
    }
  } else {
    OLP_SDK_LOG_DEBUG_F("CursorWrapper",
                        "~CursorWrapper(): Transaction was not commited, "
                        "last_result=%d",
                        last_result_);
    mdb_txn_abort(txn);
  }
}

int CursorWrapper::Get(MDB_val* key, MDB_val* value,
                       MDB_cursor_op option) const {
  return last_result_ = mdb_cursor_get(cursor_, key, value, option);
}

int CursorWrapper::Put(MDB_val* key, MDB_val* value, unsigned int flags) {
  return last_result_ = mdb_cursor_put(cursor_, key, value, flags);
}

int CursorWrapper::Put(const std::string& key, const std::string& value,
                       unsigned int flags) {
  MDB_val mkey{key.length(), (void*)key.c_str()};
  MDB_val mvalue{value.length(), (void*)value.c_str()};
  return Put(&mkey, &mvalue, flags);
}

int CursorWrapper::Put(const std::string& key,
                       const KeyValueCache::ValueTypePtr value,
                       unsigned int flags) {
  MDB_val mkey{key.length(), (void*)key.c_str()};
  MDB_val mvalue{value->size(), value->data()};
  return Put(&mkey, &mvalue, flags);
}

int CursorWrapper::Del(MDB_val* key, unsigned int flags) {
  MDB_val value;
  if (Get(key, &value, MDB_SET) != 0)
    return last_result_;
  return last_result_ = mdb_cursor_del(cursor_, flags);
}

int CursorWrapper::Del(const std::string& key, unsigned int flags) {
  MDB_val mkey{key.length(), (void*)key.c_str()};
  return Del(&mkey, flags);
}

bool CursorWrapper::IsValid() const { return cursor_ != nullptr; }

int CursorWrapper::CommitTransaction() {
  auto* txn = mdb_cursor_txn(cursor_);
  mdb_cursor_close(cursor_);
  cursor_ = nullptr;
  if (txn == nullptr) {
    return MDB_BAD_TXN;
  }
  return mdb_txn_commit(txn);
}

void CursorWrapper::AbortTransaction() {
  auto* txn = mdb_cursor_txn(cursor_);
  mdb_cursor_close(cursor_);
  cursor_ = nullptr;
  if (txn) {
    mdb_txn_abort(txn);
  }
}

int CursorWrapper::GetLastResult() const { return last_result_; }

MDB_cursor* CursorWrapper::GetCursor() { return cursor_; }

namespace {
constexpr auto kLogTag = "DiskCacheLmDb";

const std::string lmdb_error_to_string(int errcode) {
  switch (errcode) {
    case MDB_VERSION_MISMATCH:
      return "MDB_VERSION_MISMATCH";
    case MDB_INVALID:
      return "MDB_INVALID";
    case MDB_MAP_FULL:
      return "MDB_MAP_FULL";
    case MDB_BAD_TXN:
      return "MDB_BAD_TXN";
    case MDB_NOTFOUND:
      return "MDB_NOTFOUND";  // key / data pair not found(EOF)
    case MDB_DBS_FULL:
      return "MDB_DBS_FULL";  // too many databases have been opened. See
                              // mdb_env_set_maxdbs().
    case ENOENT:
      return "ENOENT";
    case EACCES:
      return "EACCES";
    case EAGAIN:
      return "EAGAIN";
    case ESRCH:
      return "ESRCH";
    case EINVAL:
      return "EINVAL";
  }
  return "Unknown";
}

}  // anonymous namespace

DiskCacheLmDb::DiskCacheLmDb(){};
DiskCacheLmDb::~DiskCacheLmDb() { Close(); }

void DiskCacheLmDb::Close() {
  // TODO: Close all transactions, cursors, etc
  // before closing environment
  if (lmdb_env_) {
    mdb_dbi_close(lmdb_env_, database_handle_);
    mdb_env_close(lmdb_env_);
    lmdb_env_ = nullptr;
  }
}

bool DiskCacheLmDb::Clear() {
  Close();

  if (!disk_cache_path_.empty()) {
    return olp::utils::Dir::remove(disk_cache_path_);
  }

  return true;
}

void DiskCacheLmDb::Compact() {
  /* No compaction needed in lmdb*/
  // mdb_env_copy2 can be executed with MDB_CP_COMPACT flag to perform
  // compaction of an environment copy. MDB_CP_COMPACT - Perform compaction
  // while copying : omit free pages and sequentially renumber all pages in
  // output.This option consumes more CPU and runs more slowly than the default.
}

OpenResult DiskCacheLmDb::Open(const std::string& data_path,
                               StorageSettings settings, OpenOptions options) {
  disk_cache_path_ = data_path;
  if (!olp::utils::Dir::exists(disk_cache_path_)) {
    if (!olp::utils::Dir::create(disk_cache_path_))
      return OpenResult::Fail;
  }

  if (lmdb_env_) {
    OLP_SDK_LOG_DEBUG(kLogTag, "Open: Trying to open already opened database");
  }

  auto status = mdb_env_create(&lmdb_env_);
  if (status != 0) {
    OLP_SDK_LOG_ERROR_F(kLogTag,
                        "Open: Failed to create environment Error Code:%s, "
                        "disk_cache_path:%s",
                        lmdb_error_to_string(status).c_str(),
                        disk_cache_path_.c_str());
    mdb_env_close(lmdb_env_);
    lmdb_env_ = nullptr;
    return OpenResult::Fail;
  }

  is_read_only_ = (options & ReadOnly) == ReadOnly;
  max_size_ = settings.max_disk_storage;  // Never used for now

  /*if (max_size_ != kSizeMax) {
    if (!check_status(mdb_env_set_mapsize(lmdb_env_, max_size_))) {
      return OpenResult::Fail;
    }
  } else { // 1MB * 100000 - default value
    if (!check_status(
            mdb_env_set_mapsize(lmdb_env_, (size_t)1048576 * (size_t)100000))) {
      return OpenResult::Fail;
    }
  }*/

  // TODO: set proper maximum size for mapping, in case max_file_size
  // is too big(kSizeMax)
  if (!is_read_only_) {  // Do not change size if we open it in ReadOnly mode
    const auto default_size = (size_t)1048576 * (size_t)100000;
    if (status =
            mdb_env_set_mapsize(lmdb_env_, default_size)) {  // 1MB * 100000
      OLP_SDK_LOG_ERROR_F(
          kLogTag,
          "Open: Failed to change database map size Error Code:%s, size:%ld",
          lmdb_error_to_string(status).c_str(), default_size);
      return OpenResult::Fail;
    }
  }

  if (status =
          mdb_env_set_maxdbs(lmdb_env_, 1)) {  // Limit to only one database
    OLP_SDK_LOG_ERROR_F(kLogTag, "Open: Failed to set max dbs Error Code:%s, ",
                        lmdb_error_to_string(status).c_str());
    return OpenResult::Fail;
  }

  // NOTE: Creating environment with MDB_RDONLY flag for
  // ReadOnly may be more effective, but it can change
  // existing behaviour. Use flags 0, control read only
  // using transactions and cursors (levelDB was using
  // ReadOnlyEnv for that purpose).
  // If 3rd parameter is non-default(0),it will use
  // CreateFileW on Windows. It will lead to and error
  // if MDB_RDONLY(dwCreationDisposition will be OPEN_EXISTING)
  // and no file exist yet.
  if (status = mdb_env_open(lmdb_env_, disk_cache_path_.c_str(), 0, 0664)) {
    OLP_SDK_LOG_ERROR_F(kLogTag,
                        "Open: Failed to open environment Error Code:%s, "
                        "disk_cache_path:%s, read_only:%d",
                        lmdb_error_to_string(status).c_str(),
                        disk_cache_path_.c_str(), is_read_only_);
    mdb_env_close(lmdb_env_);
    lmdb_env_ = nullptr;
    return OpenResult::Fail;
  }

  if (status = InitDataBaseHandle(&database_handle_, is_read_only_)) {
    OLP_SDK_LOG_ERROR_F(kLogTag,
                        "Open: Failed to initialize database Error "
                        "Code:%s, read_only:%d",
                        lmdb_error_to_string(status).c_str(), is_read_only_);
    return OpenResult::Fail;
  }

  return OpenResult::Success;
}

boost::optional<std::string> DiskCacheLmDb::Get(const std::string& key) const {
  if (!lmdb_env_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Get: Database is not initialized");
    return boost::none;
  }

  MDB_txn* txn = nullptr;
  auto status = mdb_txn_begin(lmdb_env_, nullptr, MDB_RDONLY, &txn);
  if (status != 0) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Get: Failed to begin transaction Error Code:%s, key:%s",
        lmdb_error_to_string(status).c_str(), key.c_str());

    return boost::none;
  }

  MDB_val mKey{key.length(), (void*)key.c_str()};
  MDB_val mVal;
  if (status = mdb_get(txn, database_handle_, &mKey, &mVal)) {
    if (status != MDB_NOTFOUND) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Get: Failed to get data Error Code:%s, key:%s",
                          lmdb_error_to_string(status).c_str(), key.c_str());
    }
    mdb_txn_abort(txn);
    return boost::none;
  }

  std::string str(reinterpret_cast<const char*>(mVal.mv_data),
                  mVal.mv_size);  // Copy data before commit

  // Pointer to data will be invalid after transaction commit
  if (status = mdb_txn_commit(txn)) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Get: Failed to commit transaction Error Code:%s, key:%s",
        lmdb_error_to_string(status).c_str(), key.c_str());
    mdb_txn_abort(txn);
    return boost::none;
  }

  return boost::optional<std::string>(std::move(str));
}

bool DiskCacheLmDb::Get(const std::string& key,
                        KeyValueCache::ValueTypePtr& value) const {
  if (!lmdb_env_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Get: Database is not initialized");
    return false;
  }
  value = nullptr;
  MDB_txn* txn = nullptr;
  auto status = mdb_txn_begin(lmdb_env_, nullptr, MDB_RDONLY, &txn);
  if (status != 0) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Get: Failed to begin transaction Error Code:%s, key:%s",
        lmdb_error_to_string(status).c_str(), key.c_str());
    return false;
  }

  MDB_val mKey{key.length(), (void*)key.c_str()};
  MDB_val mVal;
  if (status = mdb_get(txn, database_handle_, &mKey, &mVal)) {
    if (status != MDB_NOTFOUND) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Get: Failed to get data Error Code:%s, key:%s",
                          lmdb_error_to_string(status).c_str(), key.c_str());
    }
    mdb_txn_abort(txn);
    return false;
  }

  value = std::make_shared<KeyValueCache::ValueType>(
      reinterpret_cast<const char*>(mVal.mv_data),
      reinterpret_cast<const char*>(mVal.mv_data) + mVal.mv_size);

  // Once a transaction is closed, mKey and mVal can no longer been used
  if (status = mdb_txn_commit(txn)) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Get: Failed to commit transaction Error Code:%s, key:%s",
        lmdb_error_to_string(status).c_str(), key.c_str());
    mdb_txn_abort(txn);
    return false;
  }

  return true;
}

bool DiskCacheLmDb::Contains(const std::string& key) const {
  if (!lmdb_env_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Contains: Database is not initialized");
    return false;
  }

  auto* cursor = NewCursor(true);
  if (!cursor) {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Contains: Failed to create new curosor key:%s", key.c_str());
    return false;
  }

  auto* txn = mdb_cursor_txn(cursor);
  if (!txn) {
    mdb_cursor_close(cursor);
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Contains: Failed to get transaction from a cursor key:%s",
        key.c_str());
    return false;
  }

  MDB_val mKey{key.length(), (void*)key.c_str()};
  MDB_val data;
  if (const auto status = mdb_cursor_get(cursor, &mKey, &data, MDB_SET)) {
    if (status != MDB_NOTFOUND) {
      OLP_SDK_LOG_ERROR_F(kLogTag,
                          "Contains: Failed to get data Error Code:%s, key:%s",
                          lmdb_error_to_string(status).c_str(), key.c_str());
    }
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    return false;
  }
  mdb_cursor_close(cursor);
  mdb_txn_commit(txn);
  return true;
}

bool DiskCacheLmDb::Remove(const std::string& key,
                           uint64_t& removed_data_size) {
  removed_data_size = 0u;
  if (!lmdb_env_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Remove: Database is not initialized");
    return false;
  }

  auto* cursor = NewCursor(false);
  if (cursor == nullptr) {
    OLP_SDK_LOG_ERROR(kLogTag, "Remove: Failed to create new cursor");
    return false;
  }

  const auto abort_txn = [&]() {
    auto* txn = mdb_cursor_txn(cursor);
    mdb_cursor_close(cursor);
    if (txn) {
      mdb_txn_abort(txn);
    }
  };

  MDB_val mkey{key.size(), (void*)key.c_str()}, mval;
  auto status = mdb_cursor_get(cursor, &mkey, &mval, MDB_SET);
  if (status != 0) {
    if (status != MDB_NOTFOUND) {
      OLP_SDK_LOG_ERROR(kLogTag, "Remove: Failed to get cursor Error Code:" +
                                     lmdb_error_to_string(status));
    }
    abort_txn();
    return false;
  }

  const uint64_t data_size =
      mkey.mv_size +
      mval.mv_size;  // need to do this before commit or abort transaction
  if (status = mdb_cursor_del(cursor, 0)) {
    OLP_SDK_LOG_ERROR(kLogTag, "Remove: Failed to delete data Error Code:" +
                                   lmdb_error_to_string(status));
    abort_txn();
    return false;
  }

  auto* txn = mdb_cursor_txn(cursor);
  mdb_cursor_close(cursor);
  if (txn == nullptr) {
    OLP_SDK_LOG_ERROR(kLogTag, "Remove: Failed to get transaction");
    return false;
  }

  if (status = mdb_txn_commit(txn)) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Remove: Failed to commit transaction Error Code:" +
                          lmdb_error_to_string(status));
    return false;
  }

  removed_data_size = data_size;
  return true;
}

MDB_cursor* DiskCacheLmDb::NewCursor(bool read_only) const {
  if (!lmdb_env_) {
    OLP_SDK_LOG_ERROR(kLogTag, "NewCursor: Database is not initialized");
    return nullptr;
  }

  MDB_txn* txn = nullptr;
  // Take into count is_read_only_, so in case DB was opend
  // with ReadOnly attribute it will lead to an
  // error, when triying to make write or delete operation
  auto status = mdb_txn_begin(
      lmdb_env_, nullptr, (read_only || is_read_only_) ? MDB_RDONLY : 0, &txn);
  if (status != 0) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "NewCursor: Failed to begin new transaction Error Code:" +
                          lmdb_error_to_string(status));
    return nullptr;
  }

  MDB_cursor* new_cursor = nullptr;
  if (status = mdb_cursor_open(txn, database_handle_, &new_cursor)) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "NewCursor: Failed to open new cursor Error Code:" +
                          lmdb_error_to_string(status));
    mdb_txn_abort(txn);
    return nullptr;
  }

  return new_cursor;
}

bool DiskCacheLmDb::RemoveKeysWithPrefix(const std::string& prefix,
                                         uint64_t& removed_data_size,
                                         const RemoveFilterFunc& filter) {
  uint64_t data_size = removed_data_size = 0u;
  if (!lmdb_env_) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "RemoveKeysWithPrefix: Database is not initialized");
    return false;
  }

  // For leveldb verify_checksums option was enablead and fill_cache was
  // disabled.
  MDB_val mkey, mvalue;
  auto* cursor = SetCursorRange(prefix, &mkey, &mvalue);

  if (cursor == nullptr) {
    return false;
  }

  do {
    if (!prefix.empty()) {
      if (mkey.mv_size < prefix.size()) {
        continue;
      }
      if (prefix.compare(0, mkey.mv_size,
                         reinterpret_cast<const char*>(mkey.mv_data),
                         prefix.size()) != 0) {
        continue;
      }
    }

    const std::string key{reinterpret_cast<const char*>(mkey.mv_data),
                          mkey.mv_size};

    // Do not delete if protected
    if (filter && filter(key)) {
      continue;
    }

    if (const auto status = mdb_cursor_del(cursor, 0)) {
      OLP_SDK_LOG_ERROR(
          kLogTag, "RemoveKeysWithPrefix: Failed to delete cursor Error Code:" +
                       lmdb_error_to_string(status));
    } else {
      data_size += mvalue.mv_size + mkey.mv_size;
    }

  } while (mdb_cursor_get(cursor, &mkey, &mvalue, MDB_NEXT) == 0);

  auto* txn = mdb_cursor_txn(cursor);
  mdb_cursor_close(cursor);

  const auto result = txn ? mdb_txn_commit(txn) == 0 : false;

  if (!result) {
    data_size = 0u;
  }
  removed_data_size = data_size;

  return result;
}

uint64_t DiskCacheLmDb::Size() const {
  // TODO: Calculate size more presiselly. What size do we expect to return: db
  // file size? size of all records in db ? Size of mamory map ?
  if (!lmdb_env_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Size: Database is not initialized");
    return 0;
  }

  std::unique_ptr<MDB_stat> info{new MDB_stat()};
  if (const auto status =
          mdb_env_stat(lmdb_env_, info.get()) || info == nullptr) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Size: Failed to get environment info Error Code:" +
                          lmdb_error_to_string(status));
    return 0;
  }

  if (info->ms_entries == 0)
    return 0;

  return info->ms_entries * info->ms_psize;  // approximately

  /*std::unique_ptr<MDB_envinfo> stat{
      new MDB_envinfo()};  // allocate memory for statistic info
  if (const auto status =
          mdb_env_info(lmdb_env_, stat.get()) || stat == nullptr) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "Size: Failed to get environment info Error Code:" +
                          lmdb_error_to_string(status));
    return lmd_metainfo_size;
  }

  return stat
      ->me_mapsize;  // TODO: this will only return map size, which is basically
                     // equal to what we set in mdb_env_set_mapsize
   */
}

int DiskCacheLmDb::InitDataBaseHandle(MDB_dbi* database_handle,
                                      bool read_only) const {
  // Begin transaction, need to make it at least once to be able to read from or
  // write to db
  MDB_txn* txn = nullptr;
  auto status =
      mdb_txn_begin(lmdb_env_, nullptr, read_only ? MDB_RDONLY : 0, &txn);
  if (status != 0) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "InitDataBaseHandle: Failed to begin transaction Error "
                        "Code:%s, read_only:%d",
                        lmdb_error_to_string(status).c_str(), read_only);
    return status;
  }

  if (status = mdb_dbi_open(txn, nullptr, 0, database_handle)) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "InitDataBaseHandle: Failed to open database Error "
                        "Code:%s, read_only:%d",
                        lmdb_error_to_string(status).c_str(), read_only);
    mdb_txn_abort(txn);
    return status;
  }

  if (status = mdb_txn_commit(txn)) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "InitDataBaseHandle: Failed to commit transaction "
                        "Error Code:%s, read_only:%d",
                        lmdb_error_to_string(status).c_str(), read_only);
    return status;
  }

  return 0;
}
MDB_cursor* DiskCacheLmDb::SetCursorRange(const std::string& prefix,
                                          MDB_val* mkey,
                                          MDB_val* mvalue) const {
  auto* cursor = NewCursor(false);
  if (!cursor) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "RemoveKeysWithPrefix: Failed to create new cursor:");
    return nullptr;
  }

  const auto abort_txn = [&]() {
    auto* txn = mdb_cursor_txn(cursor);
    mdb_cursor_close(cursor);
    if (txn) {
      mdb_txn_abort(txn);
    }
  };

  if (prefix.empty()) {
    if (const auto status = mdb_cursor_get(cursor, mkey, mvalue, MDB_FIRST)) {
      OLP_SDK_LOG_WARNING(
          kLogTag, "RemoveKeysWithPrefix: Failed to get cursor Error Code:" +
                       lmdb_error_to_string(status));
      abort_txn();
      return nullptr;
    }
  } else {
    *mkey = {prefix.size(), (void*)prefix.c_str()};
    // Position at first key greater than or equal to specified key.
    // (Means it an be same key, key, that has same prefix, or any other key,
    // so we need to check mkey to be sure that it's what we are looking for)
    if (const auto status =
            mdb_cursor_get(cursor, mkey, mvalue, MDB_SET_RANGE)) {
      if (status != MDB_NOTFOUND) {
        OLP_SDK_LOG_ERROR(
            kLogTag, "RemoveKeysWithPrefix: Failed to get cursor Error Code:" +
                         lmdb_error_to_string(status));
      }
      abort_txn();
      return nullptr;
    }
  }

  return cursor;
}
}  // namespace cache
}  // namespace olp
