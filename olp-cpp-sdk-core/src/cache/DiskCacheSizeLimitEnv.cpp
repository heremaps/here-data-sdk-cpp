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

#include "DiskCacheSizeLimitEnv.h"

#include "DiskCacheSizeLimitWritableFile.h"

namespace olp {
namespace cache {
namespace {

bool IsLogFile(const std::string& name) {
  static constexpr char log_suffix[] = ".log";
  constexpr size_t log_suffix_length = 4;
  return name.compare(name.size() - log_suffix_length, log_suffix_length,
                      log_suffix) == 0;
}

char PathSeparator() {
#ifdef WIN32
  return '\\';
#else
  return '/';
#endif
}

}  // namespace

DiskCacheSizeLimitEnv::DiskCacheSizeLimitEnv(leveldb::Env* env,
                                             const std::string& base_path,
                                             bool enforce_strict_data_save)
    : SizeCountingEnv(env),
      enforce_strict_data_save_(enforce_strict_data_save) {
  std::vector<std::string> children;
  if (target()->GetChildren(base_path, &children).ok()) {
    for (const std::string& child : children) {
      uint64_t size;
      std::string full_path(base_path + PathSeparator() + child);
      if (target()->GetFileSize(full_path, &size).ok()) {
        total_size_ += size;
      }
    }
  }
}

leveldb::Status DiskCacheSizeLimitEnv::NewWritableFile(
    const std::string& f, leveldb::WritableFile** r) {
  leveldb::WritableFile* file = nullptr;
  leveldb::Status status = leveldb::Status::OK();

  if (enforce_strict_data_save_ || !IsLogFile(f)) {
    status = target()->NewWritableFile(f, &file);
  }

  if (status.ok()) {
    *r = new DiskCacheSizeLimitWritableFile(this, file);
  }

  return status;
}

leveldb::Status DiskCacheSizeLimitEnv::DeleteFile(const std::string& f) {
  uint64_t size = 0;
  if (target()->GetFileSize(f, &size).ok()) {
    total_size_ -= size;
  }
  return target()->DeleteFile(f);
}

void DiskCacheSizeLimitEnv::AddSize(size_t size) { total_size_ += size; }

uint64_t DiskCacheSizeLimitEnv::Size() const { return total_size_.load(); }

}  // namespace cache
}  // namespace olp
