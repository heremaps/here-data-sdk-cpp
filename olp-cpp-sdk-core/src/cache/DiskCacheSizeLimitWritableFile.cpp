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

#include "DiskCacheSizeLimitWritableFile.h"
#include "DiskCacheSizeLimitEnv.h"

namespace olp {
namespace cache {

DiskCacheSizeLimitWritableFile::DiskCacheSizeLimitWritableFile(
    DiskCacheSizeLimitEnv* owner, std::unique_ptr<rocksdb::WritableFile> file)
    : owner_(owner), file_(std::move(file)) {}

rocksdb::Status DiskCacheSizeLimitWritableFile::Append(
    const rocksdb::Slice& data) {
  if (file_) {
    owner_->AddSize(data.size());
    return file_->Append(data);
  }
  return rocksdb::Status::OK();
}

rocksdb::Status DiskCacheSizeLimitWritableFile::Close() {
  if (file_) {
    return file_->Close();
  }
  return rocksdb::Status::OK();
}

rocksdb::Status DiskCacheSizeLimitWritableFile::Flush() {
  if (file_) {
    return file_->Flush();
  }
  return rocksdb::Status::OK();
}

rocksdb::Status DiskCacheSizeLimitWritableFile::Sync() {
  if (file_) {
    return file_->Sync();
  }
  return rocksdb::Status::OK();
}
}  // namespace cache
}  // namespace olp
