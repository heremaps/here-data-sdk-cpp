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

#include <rocksdb/env.h>

namespace olp {
namespace cache {

class DiskCacheSizeLimitEnv : public rocksdb::EnvWrapper {
 public:
  // Initialize an EnvWrapper that delegates all calls to *t
  DiskCacheSizeLimitEnv(rocksdb::Env* env, const std::string& base_path,
                        bool enforce_strict_data_save);

  rocksdb::Status NewWritableFile(const std::string& f,
                                  std::unique_ptr<rocksdb::WritableFile>* r,
                                  const rocksdb::EnvOptions& options) override;

  rocksdb::Status DeleteFile(const std::string& f) override;

  void AddSize(size_t size);

  uint64_t Size() const;

 private:
  std::atomic<uint64_t> total_size_{0};
  bool enforce_strict_data_save_{false};
};

}  // namespace cache
}  // namespace olp
