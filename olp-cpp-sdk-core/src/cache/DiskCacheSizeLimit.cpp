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

#include "DiskCacheSizeLimit.h"

namespace olp {
namespace cache {
namespace {
bool isLogFile(const std::string& fname) {
  static constexpr char logSuffix[] = ".log";
  constexpr size_t logSuffixLength = 4;
  return fname.compare(fname.size() - logSuffixLength, logSuffixLength,
                       logSuffix) == 0;
}
}  // namespace

leveldb::Status DiskCacheSizeLimit::NewWritableFile(const std::string& f,
                                                    leveldb::WritableFile** r) {
  leveldb::WritableFile* file = nullptr;
  leveldb::Status status = leveldb::Status::OK();

  if (m_enforceStrictDataSave || !isLogFile(f)) {
    status = m_env->NewWritableFile(f, &file);
  }

  if (status.ok()) *r = new DisckCacheSizeLimitWritableFile(this, file);
  return status;
}

}  // namespace cache
}  // namespace olp
