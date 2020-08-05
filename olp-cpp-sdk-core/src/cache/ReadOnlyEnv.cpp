/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "ReadOnlyEnv.h"

namespace olp {
namespace cache {
namespace {

class FakeWritableFile : public leveldb::WritableFile {
 public:
  ~FakeWritableFile() override = default;

  leveldb::Status Append(const leveldb::Slice& /* data */) override {
    return {};
  }

  leveldb::Status Close() override { return {}; }

  leveldb::Status Flush() override { return {}; }

  leveldb::Status Sync() override { return {}; }
};

}  // namespace

ReadOnlyEnv::ReadOnlyEnv(leveldb::Env* env) : SizeCountingEnv(env) {}

leveldb::Status ReadOnlyEnv::NewWritableFile(const std::string& /* f */,
                                             leveldb::WritableFile** r) {
  *r = new FakeWritableFile();
  return {};
}

leveldb::Status ReadOnlyEnv::NewAppendableFile(const std::string& /* f */,
                                               leveldb::WritableFile** r) {
  *r = new FakeWritableFile();
  return {};
}

leveldb::Status ReadOnlyEnv::LockFile(const std::string& /* f */,
                                      leveldb::FileLock** /* l */) {
  return {};
}

leveldb::Status ReadOnlyEnv::UnlockFile(leveldb::FileLock* /* l */) {
  return {};
}

leveldb::Status ReadOnlyEnv::RenameFile(const std::string& /* s */,
                                        const std::string& /* t */) {
  return {};
}

}  // namespace cache
}  // namespace olp
