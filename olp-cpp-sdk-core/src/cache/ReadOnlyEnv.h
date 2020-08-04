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

#pragma once

#include "SizeCountingEnv.h"

namespace olp {
namespace cache {

class ReadOnlyEnv : public SizeCountingEnv {
 public:
  // Initialize an EnvWrapper that delegates all calls to *t
  explicit ReadOnlyEnv(leveldb::Env* env);
  ~ReadOnlyEnv() override = default;

  leveldb::Status NewWritableFile(const std::string& f,
                                  leveldb::WritableFile** r) override;

  leveldb::Status NewAppendableFile(const std::string& f,
                                    leveldb::WritableFile** r) override;

  leveldb::Status LockFile(const std::string& f,
                           leveldb::FileLock** l) override;
  leveldb::Status UnlockFile(leveldb::FileLock* l) override;

  leveldb::Status RenameFile(const std::string& s,
                             const std::string& t) override;
};

}  // namespace cache
}  // namespace olp
