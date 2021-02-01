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

#include "DiskCacheSizeLimitWritableFile.h"
#include "DiskCacheSizeLimitEnv.h"

namespace olp {
namespace cache {

DiskCacheSizeLimitWritableFile::DiskCacheSizeLimitWritableFile(
    DiskCacheSizeLimitEnv* owner, leveldb::WritableFile* file)
    : owner_(owner), file_(file) {}

leveldb::Status DiskCacheSizeLimitWritableFile::Append(
    const leveldb::Slice& data) {
  if (file_) {
    owner_->AddSize(data.size());
    return file_->Append(data);
  }
  return leveldb::Status::OK();
}

leveldb::Status DiskCacheSizeLimitWritableFile::Close() {
  if (file_) {
    return file_->Close();
  }
  return leveldb::Status::OK();
}

leveldb::Status DiskCacheSizeLimitWritableFile::Flush() {
  if (file_) {
    return file_->Flush();
  }
  return leveldb::Status::OK();
}

leveldb::Status DiskCacheSizeLimitWritableFile::Sync() {
  if (file_) {
    return file_->Sync();
  }
  return leveldb::Status::OK();
}

}  // namespace cache
}  // namespace olp
