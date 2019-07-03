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

#include <atomic>
#include <cstdint>
#include <memory>

#include <leveldb/env.h>

// The DeleteFile macro from windows.h conflicts with
// the DeleteFile method from leveldb::Env
// Since we are subclassing leveldb::Env, we have to hide the
// macro for this file (following the pattern from leveldb).
// See env.h inside leveldb for more details.
#if defined(_WIN32)
#if defined(DeleteFile)
#undef DeleteFile
#define LEVELDB_DELETEFILE_UNDEFINED
#endif  // defined(DeleteFile)
#endif  // defined(_WIN32)

namespace olp {
namespace cache {

class DiskCacheSizeLimit;

class DisckCacheSizeLimitWritableFile : public leveldb::WritableFile {
 public:
  DisckCacheSizeLimitWritableFile(DiskCacheSizeLimit* owner,
                                  leveldb::WritableFile* file)
      : m_owner(owner), m_file(file) {}
  ~DisckCacheSizeLimitWritableFile() override = default;

  inline leveldb::Status Append(const leveldb::Slice& data) override;

  leveldb::Status Close() override {
    if (!m_file) {
      return leveldb::Status::OK();
    } else {
      return m_file->Close();
    }
  }

  leveldb::Status Flush() override {
    if (!m_file) {
      return leveldb::Status::OK();
    } else {
      return m_file->Flush();
    }
  }

  leveldb::Status Sync() override {
    if (!m_file) {
      return leveldb::Status::OK();
    } else {
      return m_file->Sync();
    }
  }

 private:
  DiskCacheSizeLimit* m_owner{nullptr};
  std::unique_ptr<leveldb::WritableFile> m_file;
};

class DiskCacheSizeLimit : public leveldb::Env {
 public:
  // Initialize an EnvWrapper that delegates all calls to *t
  DiskCacheSizeLimit(leveldb::Env* env, const std::string& basePath,
                     bool enforceStrictDataSave)
      : m_env(env),
        m_basePath(basePath),
        m_enforceStrictDataSave(enforceStrictDataSave) {
    std::vector<std::string> children;
    if (m_env->GetChildren(m_basePath, &children).ok()) {
      for (const std::string& child : children) {
        uint64_t size;
        std::string fullPath(m_basePath + PathSeparator + child);
        if (m_env->GetFileSize(fullPath, &size).ok()) m_totalSize += size;
      }
    }
  }

  // The following text is boilerplate that forwards all methods to target()
  leveldb::Status NewSequentialFile(const std::string& f,
                                    leveldb::SequentialFile** r) override {
    return m_env->NewSequentialFile(f, r);
  }

  leveldb::Status NewRandomAccessFile(const std::string& f,
                                      leveldb::RandomAccessFile** r) override {
    return m_env->NewRandomAccessFile(f, r);
  }

  leveldb::Status NewWritableFile(const std::string& f,
                                  leveldb::WritableFile** r) override;

  bool FileExists(const std::string& f) override {
    return m_env->FileExists(f);
  }

  leveldb::Status GetChildren(const std::string& dir,
                              std::vector<std::string>* r) override {
    return m_env->GetChildren(dir, r);
  }

  leveldb::Status DeleteFile(const std::string& f) override {
    uint64_t size = 0;
    if (m_env->GetFileSize(f, &size).ok()) m_totalSize -= size;
    return m_env->DeleteFile(f);
  }

  leveldb::Status CreateDir(const std::string& d) override {
    return m_env->CreateDir(d);
  }

  leveldb::Status DeleteDir(const std::string& d) override {
    return m_env->DeleteDir(d);
  }

  leveldb::Status GetFileSize(const std::string& f, uint64_t* s) override {
    return m_env->GetFileSize(f, s);
  }

  leveldb::Status RenameFile(const std::string& s,
                             const std::string& t) override {
    return m_env->RenameFile(s, t);
  }

  leveldb::Status LockFile(const std::string& f,
                           leveldb::FileLock** l) override {
    return m_env->LockFile(f, l);
  }

  leveldb::Status UnlockFile(leveldb::FileLock* l) override {
    return m_env->UnlockFile(l);
  }

  void Schedule(void (*f)(void*), void* a) override {
    return m_env->Schedule(f, a);
  }

  void StartThread(void (*f)(void*), void* a) override {
    return m_env->StartThread(f, a);
  }

  leveldb::Status GetTestDirectory(std::string* path) override {
    return m_env->GetTestDirectory(path);
  }

  leveldb::Status NewLogger(const std::string& fname,
                            leveldb::Logger** result) override {
    return m_env->NewLogger(fname, result);
  }

  uint64_t NowMicros() override { return m_env->NowMicros(); }

  void SleepForMicroseconds(int micros) override {
    m_env->SleepForMicroseconds(micros);
  }

  void addSize(size_t size) { m_totalSize += size; }

  uint64_t size() const { return m_totalSize.load(); }

 private:
  leveldb::Env* m_env{nullptr};
  std::string m_basePath;
  std::atomic<uint64_t> m_totalSize{0};
  bool m_enforceStrictDataSave{false};

#ifdef WIN32
  static const char PathSeparator = '\\';
#else
  static const char PathSeparator = '/';
#endif
};

inline leveldb::Status DisckCacheSizeLimitWritableFile::Append(
    const leveldb::Slice& data) {
  if (!m_file) {
    return leveldb::Status::OK();
  }
  m_owner->addSize(data.size());
  return m_file->Append(data);
}
}  // namespace cache
}  // namespace olp

#if defined(_WIN32) && defined(LEVELDB_DELETEFILE_UNDEFINED)
#if defined(UNICODE)
#define DeleteFile DeleteFileW
#else
#define DeleteFile DeleteFileA
#endif  // defined(UNICODE)
#endif  // defined(_WIN32) && defined(LEVELDB_DELETEFILE_UNDEFINED)
