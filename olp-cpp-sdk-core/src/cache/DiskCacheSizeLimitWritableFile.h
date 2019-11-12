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

#include <memory>

#include <rocksdb/env.h>

namespace olp {
namespace cache {
class DiskCacheSizeLimitEnv;

class DiskCacheSizeLimitWritableFile : public rocksdb::WritableFile {
 public:
  DiskCacheSizeLimitWritableFile(DiskCacheSizeLimitEnv* owner,
                                 std::unique_ptr<rocksdb::WritableFile> file);
  ~DiskCacheSizeLimitWritableFile() override = default;

  rocksdb::Status Append(const rocksdb::Slice& data) override;

  rocksdb::Status Close() override;

  rocksdb::Status Flush() override;

  rocksdb::Status Sync() override;

 private:
  DiskCacheSizeLimitEnv* owner_{nullptr};
  std::unique_ptr<rocksdb::WritableFile> file_;
};

}  // namespace cache
}  // namespace olp
