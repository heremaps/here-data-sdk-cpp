/*
 * Copyright (C) 2021-2023 HERE Europe B.V.
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
#include <olp/core/utils/WarningWorkarounds.h>

#ifndef PORTING_PLATFORM_WINDOWS

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#if defined(HAVE_O_CLOEXEC)
constexpr const int kOpenBaseFlags = O_CLOEXEC;
#else
constexpr const int kOpenBaseFlags = 0;
#endif  // defined(HAVE_O_CLOEXEC)

namespace {

constexpr const size_t kWritableFileBufferSize = 65536;

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
      fd = ::open(filename_.c_str(), O_RDONLY | kOpenBaseFlags);
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

// Copy from leveldb
class PosixWritableFile final : public leveldb::WritableFile {
 public:
  PosixWritableFile(std::string filename, int fd)
      : pos_(0),
        fd_(fd),
        is_manifest_(IsManifest(filename)),
        filename_(std::move(filename)),
        dirname_(Dirname(filename_)) {}

  ~PosixWritableFile() override {
    if (fd_ >= 0) {
      // Ignoring any potential errors
      Close();
    }
  }

  leveldb::Status Append(const leveldb::Slice& data) override {
    size_t write_size = data.size();
    const char* write_data = data.data();

    // Fit as much as possible into buffer.
    size_t copy_size = std::min(write_size, kWritableFileBufferSize - pos_);
    std::memcpy(buf_ + pos_, write_data, copy_size);
    write_data += copy_size;
    write_size -= copy_size;
    pos_ += copy_size;
    if (write_size == 0) {
      return leveldb::Status::OK();
    }

    // Can't fit in buffer, so need to do at least one write.
    leveldb::Status status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }

    // Small writes go to buffer, large writes are written directly.
    if (write_size < kWritableFileBufferSize) {
      std::memcpy(buf_, write_data, write_size);
      pos_ = write_size;
      return leveldb::Status::OK();
    }
    return WriteUnbuffered(write_data, write_size);
  }

  leveldb::Status Close() override {
    leveldb::Status status = FlushBuffer();
    const int close_result = ::close(fd_);
    if (close_result < 0 && status.ok()) {
      status = PosixError(filename_, errno);
    }
    fd_ = -1;
    return status;
  }

  leveldb::Status Flush() override { return FlushBuffer(); }

  leveldb::Status Sync() override {
    // Ensure new files referred to by the manifest are in the filesystem.
    //
    // This needs to happen before the manifest file is flushed to disk, to
    // avoid crashing in a state where the manifest refers to files that are not
    // yet on disk.
    leveldb::Status status = SyncDirIfManifest();
    if (!status.ok()) {
      return status;
    }

    status = FlushBuffer();
    if (!status.ok()) {
      return status;
    }

    return SyncFd(fd_, filename_);
  }

 private:
  leveldb::Status FlushBuffer() {
    leveldb::Status status = WriteUnbuffered(buf_, pos_);
    pos_ = 0;
    return status;
  }

  leveldb::Status WriteUnbuffered(const char* data, size_t size) {
    while (size > 0) {
      ssize_t write_result = ::write(fd_, data, size);
      if (write_result < 0) {
        if (errno == EINTR) {
          continue;  // Retry
        }
        return PosixError(filename_, errno);
      }
      data += write_result;
      size -= write_result;
    }
    return leveldb::Status::OK();
  }

  leveldb::Status SyncDirIfManifest() {
    leveldb::Status status;
    if (!is_manifest_) {
      return status;
    }

    int fd = ::open(dirname_.c_str(), O_RDONLY | kOpenBaseFlags);
    if (fd < 0) {
      status = PosixError(dirname_, errno);
    } else {
      status = SyncFd(fd, dirname_);
      ::close(fd);
    }
    return status;
  }

  // Ensures that all the caches associated with the given file descriptor's
  // data are flushed all the way to durable media, and can withstand power
  // failures.
  //
  // The path argument is only used to populate the description string in the
  // returned Status if an error occurs.
  static leveldb::Status SyncFd(int fd, const std::string& fd_path) {
#if HAVE_FULLFSYNC
    // On macOS and iOS, fsync() doesn't guarantee durability past power
    // failures. fcntl(F_FULLFSYNC) is required for that purpose. Some
    // filesystems don't support fcntl(F_FULLFSYNC), and require a fallback to
    // fsync().
    if (::fcntl(fd, F_FULLFSYNC) == 0) {
      return leveldb::Status::OK();
    }
#endif  // HAVE_FULLFSYNC

#if HAVE_FDATASYNC
    bool sync_success = ::fdatasync(fd) == 0;
#else
    bool sync_success = ::fsync(fd) == 0;
#endif  // HAVE_FDATASYNC

    if (sync_success) {
      return leveldb::Status::OK();
    }
    return PosixError(fd_path, errno);
  }

  // Returns the directory name in a path pointing to a file.
  //
  // Returns "." if the path does not contain any directory separator.
  static std::string Dirname(const std::string& filename) {
    std::string::size_type separator_pos = filename.rfind('/');
    if (separator_pos == std::string::npos) {
      return std::string(".");
    }
    // The filename component should not contain a path separator. If it does,
    // the splitting was done incorrectly.
    assert(filename.find('/', separator_pos + 1) == std::string::npos);

    return filename.substr(0, separator_pos);
  }

  // Extracts the file name from a path pointing to a file.
  //
  // The returned Slice points to |filename|'s data buffer, so it is only valid
  // while |filename| is alive and unchanged.
  static leveldb::Slice Basename(const std::string& filename) {
    std::string::size_type separator_pos = filename.rfind('/');
    if (separator_pos == std::string::npos) {
      return leveldb::Slice(filename);
    }
    // The filename component should not contain a path separator. If it does,
    // the splitting was done incorrectly.
    assert(filename.find('/', separator_pos + 1) == std::string::npos);

    return leveldb::Slice(filename.data() + separator_pos + 1,
                          filename.length() - separator_pos - 1);
  }

  // True if the given file is a manifest file.
  static bool IsManifest(const std::string& filename) {
    return Basename(filename).starts_with("MANIFEST");
  }

  // buf_[0, pos_ - 1] contains data to be written to fd_.
  char buf_[kWritableFileBufferSize];
  size_t pos_;
  int fd_;

  const bool is_manifest_;  // True if the file's name starts with MANIFEST.
  const std::string filename_;
  const std::string dirname_;  // The directory of filename_.
};

int LockOrUnlock(int fd, bool lock) {
  errno = 0;
  struct ::flock file_lock_info;
  std::memset(&file_lock_info, 0, sizeof(file_lock_info));
  file_lock_info.l_type = (lock ? F_WRLCK : F_UNLCK);
  file_lock_info.l_whence = SEEK_SET;
  file_lock_info.l_start = 0;
  file_lock_info.l_len = 0;  // Lock/unlock entire file.
  return ::fcntl(fd, F_SETLK, &file_lock_info);
}

// Instances are thread-safe because they are immutable.
class PosixFileLock : public leveldb::FileLock {
 public:
  PosixFileLock(int fd, std::string filename)
      : fd_(fd), filename_(std::move(filename)) {}

  int fd() const { return fd_; }
  const std::string& filename() const { return filename_; }

 private:
  const int fd_;
  const std::string filename_;
};

// Tracks the files locked by PosixEnv::LockFile().
//
// We maintain a separate set instead of relying on fcntrl(F_SETLK) because
// fcntl(F_SETLK) does not provide any protection against multiple uses from the
// same process.
//
// Instances are thread-safe because all member data is guarded by a mutex.
class PosixLockTable {
 public:
  bool Insert(const std::string& fname) {
    std::lock_guard<std::mutex> lock(mu_);
    return locked_files_.insert(fname).second;
  }

  void Remove(const std::string& fname) {
    std::lock_guard<std::mutex> lock(mu_);
    locked_files_.erase(fname);
  }

 private:
  std::mutex mu_;
  std::set<std::string> locked_files_;
};

/// Leveldb posix logger
class PosixLogger final : public leveldb::Logger {
 public:
  // Creates a logger that writes to the given file.
  //
  // The PosixLogger instance takes ownership of the file handle.
  explicit PosixLogger(std::FILE* fp) : fp_(fp) { assert(fp != nullptr); }

  ~PosixLogger() override { std::fclose(fp_); }

  void Logv(const char* format, std::va_list arguments) override {
    // Record the time as close to the Logv() call as possible.
    struct ::timeval now_timeval;
    ::gettimeofday(&now_timeval, nullptr);
    const std::time_t now_seconds = now_timeval.tv_sec;
    struct std::tm now_components;
    ::localtime_r(&now_seconds, &now_components);

    // Record the thread ID.
    constexpr const int kMaxThreadIdSize = 32;
    std::ostringstream thread_stream;
    thread_stream << std::this_thread::get_id();
    std::string thread_id = thread_stream.str();
    if (thread_id.size() > kMaxThreadIdSize) {
      thread_id.resize(kMaxThreadIdSize);
    }

    // We first attempt to print into a stack-allocated buffer. If this attempt
    // fails, we make a second attempt with a dynamically allocated buffer.
    constexpr const int kStackBufferSize = 512;
    char stack_buffer[kStackBufferSize];
    static_assert(sizeof(stack_buffer) == static_cast<size_t>(kStackBufferSize),
                  "sizeof(char) is expected to be 1 in C++");

    int dynamic_buffer_size = 0;  // Computed in the first iteration.
    for (int iteration = 0; iteration < 2; ++iteration) {
      const int buffer_size =
          (iteration == 0) ? kStackBufferSize : dynamic_buffer_size;
      char* const buffer =
          (iteration == 0) ? stack_buffer : new char[dynamic_buffer_size];

      // Print the header into the buffer.
      int buffer_offset = std::snprintf(
          buffer, buffer_size, "%04d/%02d/%02d-%02d:%02d:%02d.%06d %s ",
          now_components.tm_year + 1900, now_components.tm_mon + 1,
          now_components.tm_mday, now_components.tm_hour, now_components.tm_min,
          now_components.tm_sec, static_cast<int>(now_timeval.tv_usec),
          thread_id.c_str());

      // The header can be at most 28 characters (10 date + 15 time +
      // 3 delimiters) plus the thread ID, which should fit comfortably into the
      // static buffer.
      assert(buffer_offset <= 28 + kMaxThreadIdSize);
      static_assert(28 + kMaxThreadIdSize < kStackBufferSize,
                    "stack-allocated buffer may not fit the message header");
      assert(buffer_offset < buffer_size);

      // Print the message into the buffer.
      std::va_list arguments_copy;
      va_copy(arguments_copy, arguments);
      buffer_offset +=
          std::vsnprintf(buffer + buffer_offset, buffer_size - buffer_offset,
                         format, arguments_copy);
      va_end(arguments_copy);

      // The code below may append a newline at the end of the buffer, which
      // requires an extra character.
      if (buffer_offset >= buffer_size - 1) {
        // The message did not fit into the buffer.
        if (iteration == 0) {
          // Re-run the loop and use a dynamically-allocated buffer. The buffer
          // will be large enough for the log message, an extra newline and a
          // null terminator.
          dynamic_buffer_size = buffer_offset + 2;
          continue;
        }

        // The dynamically-allocated buffer was incorrectly sized. This should
        // not happen, assuming a correct implementation of std::(v)snprintf.
        // Fail in tests, recover by truncating the log message in production.
        assert(false);
        buffer_offset = buffer_size - 1;
      }

      // Add a newline if necessary.
      if (buffer[buffer_offset - 1] != '\n') {
        buffer[buffer_offset] = '\n';
        ++buffer_offset;
      }

      assert(buffer_offset <= buffer_size);
      std::fwrite(buffer, 1, buffer_offset, fp_);
      std::fflush(fp_);

      if (iteration != 0) {
        delete[] buffer;
      }
      break;
    }
  }

 private:
  std::FILE* const fp_;
};

class EnvWrapper : public leveldb::EnvWrapper {
 public:
  EnvWrapper(std::shared_ptr<PosixLockTable> locks,
             std::shared_ptr<Limiter> fd_limiter, bool extend_permissions)
      : leveldb::EnvWrapper(leveldb::Env::Default()),
        locks_(locks),
        fd_limiter_(fd_limiter),
        default_file_permissions_(extend_permissions ? DEFFILEMODE : 0644),
        default_dir_permissions_(extend_permissions ? ACCESSPERMS : 0755) {}

  ~EnvWrapper() = default;

  leveldb::Status NewRandomAccessFile(
      const std::string& filename,
      leveldb::RandomAccessFile** result) override {
    *result = nullptr;
    int fd = ::open(filename.c_str(), O_RDONLY | kOpenBaseFlags);
    if (fd < 0) {
      return PosixError(filename, errno);
    }

    *result = new PosixRandomAccessFile(filename, fd, fd_limiter_.get());
    return leveldb::Status::OK();
  }

  leveldb::Status NewWritableFile(const std::string& filename,
                                  leveldb::WritableFile** result) override {
    int fd =
        ::open(filename.c_str(), O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags,
               default_file_permissions_);
    if (fd < 0) {
      *result = nullptr;
      return PosixError(filename, errno);
    }

    *result = new PosixWritableFile(filename, fd);
    return leveldb::Status::OK();
  }

  leveldb::Status NewAppendableFile(const std::string& filename,
                                    leveldb::WritableFile** result) override {
    int fd =
        ::open(filename.c_str(), O_APPEND | O_WRONLY | O_CREAT | kOpenBaseFlags,
               default_file_permissions_);
    if (fd < 0) {
      *result = nullptr;
      return PosixError(filename, errno);
    }

    *result = new PosixWritableFile(filename, fd);
    return leveldb::Status::OK();
  }

  leveldb::Status CreateDir(const std::string& dirname) override {
    // Use maximum permissions allowed
    if (::mkdir(dirname.c_str(), default_dir_permissions_) != 0) {
      return PosixError(dirname, errno);
    }
    return leveldb::Status::OK();
  }

  leveldb::Status LockFile(const std::string& filename,
                           leveldb::FileLock** lock) override {
    *lock = nullptr;

    int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | kOpenBaseFlags,
                    default_file_permissions_);
    if (fd < 0) {
      return PosixError(filename, errno);
    }

    if (!locks_->Insert(filename)) {
      ::close(fd);
      return leveldb::Status::IOError("lock " + filename,
                                      "already held by process");
    }

    if (LockOrUnlock(fd, true) == -1) {
      int lock_errno = errno;
      ::close(fd);
      locks_->Remove(filename);
      return PosixError("lock " + filename, lock_errno);
    }

    *lock = new PosixFileLock(fd, filename);
    return leveldb::Status::OK();
  }

  leveldb::Status UnlockFile(leveldb::FileLock* lock) override {
    PosixFileLock* posix_file_lock = static_cast<PosixFileLock*>(lock);
    if (LockOrUnlock(posix_file_lock->fd(), false) == -1) {
      return PosixError("unlock " + posix_file_lock->filename(), errno);
    }
    locks_->Remove(posix_file_lock->filename());
    ::close(posix_file_lock->fd());
    delete posix_file_lock;
    return leveldb::Status::OK();
  }

  leveldb::Status NewLogger(const std::string& filename,
                            leveldb::Logger** result) override {
    int fd =
        ::open(filename.c_str(), O_APPEND | O_WRONLY | O_CREAT | kOpenBaseFlags,
               default_file_permissions_);
    if (fd < 0) {
      *result = nullptr;
      return PosixError(filename, errno);
    }

    std::FILE* fp = ::fdopen(fd, "w");
    if (fp == nullptr) {
      ::close(fd);
      *result = nullptr;
      return PosixError(filename, errno);
    } else {
      *result = new PosixLogger(fp);
      return leveldb::Status::OK();
    }
  }

 private:
  std::shared_ptr<PosixLockTable> locks_;
  std::shared_ptr<Limiter> fd_limiter_;
  const int default_file_permissions_;
  const int default_dir_permissions_;
};

}  // namespace

#endif  // PORTING_PLATFORM_WINDOWS

namespace olp {
namespace cache {

std::shared_ptr<leveldb::Env> DiskCacheEnv::CreateEnv(
    bool extend_permissions) {
#ifdef PORTING_PLATFORM_WINDOWS
  OLP_SDK_CORE_UNUSED(extend_permissions);
  // return the normal env as a shared ptr with empty deleter
  return std::shared_ptr<leveldb::Env>(leveldb::Env::Default(),
                                       [](leveldb::Env*) {});
#else
  static auto locks = std::make_shared<PosixLockTable>();
  static auto fd_limiter = std::make_shared<Limiter>(MaxOpenFiles());
  return std::make_shared<EnvWrapper>(locks, fd_limiter, extend_permissions);
#endif  // PORTING_PLATFORM_WINDOWS
}

}  // namespace cache
}  // namespace olp
