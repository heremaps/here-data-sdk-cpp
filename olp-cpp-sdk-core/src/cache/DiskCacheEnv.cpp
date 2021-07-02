/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include "DiskCacheEnv.h"

#include <olp/core/porting/platform.h>

#ifndef PORTING_PLATFORM_WINDOWS

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <thread>

namespace {

leveldb::Status PosixError(const std::string& context, int error_number) {
  if (error_number == ENOENT) {
    return leveldb::Status::NotFound(context, std::strerror(error_number));
  } else {
    return leveldb::Status::IOError(context, std::strerror(error_number));
  }
}

// Return the maximum number of read-only files to keep open.
int MaxOpenFiles() {
  static int open_read_only_file_limit = -1;
  if (open_read_only_file_limit >= 0) {
    return open_read_only_file_limit;
  }
  struct ::rlimit rlim;
  if (::getrlimit(RLIMIT_NOFILE, &rlim)) {
    // getrlimit failed, fallback to hard-coded default.
    open_read_only_file_limit = 50;
  } else if (rlim.rlim_cur == RLIM_INFINITY) {
    open_read_only_file_limit = std::numeric_limits<int>::max();
  } else {
    // Allow use of 20% of available file descriptors for read-only files.
    open_read_only_file_limit = static_cast<int>(rlim.rlim_cur) / 5;
  }
  return open_read_only_file_limit;
}

class Limiter {
 public:
  // Limit maximum number of resources to |max_acquires|.
  explicit Limiter(int max_acquires) : acquires_allowed_(max_acquires) {}

  Limiter(const Limiter&) = delete;
  Limiter operator=(const Limiter&) = delete;

  // If another resource is available, acquire it and return true.
  // Else return false.
  bool Acquire() {
    int old_acquires_allowed =
        acquires_allowed_.fetch_sub(1, std::memory_order_relaxed);

    if (old_acquires_allowed > 0)
      return true;

    acquires_allowed_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  // Release a resource acquired by a previous call to Acquire() that returned
  // true.
  void Release() { acquires_allowed_.fetch_add(1, std::memory_order_relaxed); }

 private:
  // The number of available resources.
  //
  // This is a counter and is not tied to the invariants of any other class, so
  // it can be operated on safely using std::memory_order_relaxed.
  std::atomic<int> acquires_allowed_;
};

// Implements random read access in a file using pread().
//
// Instances of this class are thread-safe, as required by the RandomAccessFile
// API. Instances are immutable and Read() only calls thread-safe library
// functions.
class PosixRandomAccessFile final : public leveldb::RandomAccessFile {
 public:
  // The new instance takes ownership of |fd|. |fd_limiter| must outlive this
  // instance, and will be used to determine if .
  PosixRandomAccessFile(std::string filename, int fd, Limiter* fd_limiter)
      : has_permanent_fd_(fd_limiter->Acquire()),
        fd_(has_permanent_fd_ ? fd : -1),
        filename_(std::move(filename)),
        fd_limiter_(fd_limiter) {
    if (!has_permanent_fd_) {
      assert(fd_ == -1);
      ::close(fd);  // The file will be opened on every read.
    }
  }

  ~PosixRandomAccessFile() override {
    if (has_permanent_fd_) {
      assert(fd_ != -1);
      ::close(fd_);
      fd_limiter_->Release();
    }
  }

  leveldb::Status Read(uint64_t offset, size_t n, leveldb::Slice* result,
                       char* scratch) const override {
    int fd = fd_;

    if (!has_permanent_fd_) {
      fd = ::open(filename_.c_str(), O_RDONLY);
      if (fd < 0) {
        return PosixError(filename_, errno);
      }
    }

    assert(fd != -1);

    leveldb::Status status;
    ssize_t read_size = ::pread(fd, scratch, n, static_cast<off_t>(offset));
    *result = leveldb::Slice(scratch, (read_size < 0) ? 0 : read_size);
    if (read_size < 0) {
      // An error: return a non-ok status.
      status = PosixError(filename_, errno);
    }

    if (!has_permanent_fd_) {
      // Close the temporary file descriptor opened earlier.
      assert(fd != fd_);
      ::close(fd);
    }

    return status;
  }

 private:
  /// If false, the file is opened on every read.
  const bool has_permanent_fd_;
  /// File descriptor, -1 if has_permanent_fd_ is false.
  const int fd_;
  /// File name
  const std::string filename_;

  Limiter* const fd_limiter_;
};

class EnvWrapper : public leveldb::EnvWrapper {
 public:
  EnvWrapper()
      : leveldb::EnvWrapper(leveldb::Env::Default()),
        fd_limiter_(MaxOpenFiles()) {}

  ~EnvWrapper() = default;

  leveldb::Status NewRandomAccessFile(
      const std::string& filename,
      leveldb::RandomAccessFile** result) override {
    *result = nullptr;
    int fd = ::open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
      return PosixError(filename, errno);
    }

    *result = new PosixRandomAccessFile(filename, fd, &fd_limiter_);
    return leveldb::Status::OK();
  }

 private:
  Limiter fd_limiter_;
};

}  // namespace

#endif  // PORTING_PLATFORM_WINDOWS

namespace olp {
namespace cache {

std::shared_ptr<leveldb::Env> DiskCacheEnv::CreateEnv() {
#ifdef PORTING_PLATFORM_WINDOWS
  // return the normal env as a shared ptr with empty deleter
  return std::shared_ptr<leveldb::Env>(leveldb::Env::Default(),
                                       [](leveldb::Env*) {});
#else
  // Static shared_ptr is needed to share the Limiter variable, and to avoid the
  // scenario when the env is destroyed at exit while the disk cache instance is
  // still kept by somebody. So the env instance will be alive until it is
  // needed.
  static auto env = std::make_shared<EnvWrapper>();
  return env;
#endif  // PORTING_PLATFORM_WINDOWS
}

}  // namespace cache
}  // namespace olp
