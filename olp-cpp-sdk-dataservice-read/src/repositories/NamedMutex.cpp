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

#include "NamedMutex.h"

#include <olp/core/porting/make_unique.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

class NamedMutexStorage::Impl {
 public:
  std::mutex& AquireLock(const std::string& resource);
  void ReleaseLock(const std::string& resource);

 private:
  struct RefCounterMutex {
    std::mutex mutex;
    uint32_t use_count{0u};
  };

  std::mutex mutex_;
  std::unordered_map<std::string, RefCounterMutex> mutexes_;
};

std::mutex& NamedMutexStorage::Impl::AquireLock(const std::string& resource) {
  std::lock_guard<std::mutex> lock(mutex_);
  RefCounterMutex& ref_mutex = mutexes_[resource];
  ref_mutex.use_count++;
  return ref_mutex.mutex;
}

void NamedMutexStorage::Impl::ReleaseLock(const std::string& resource) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto mutex_it = mutexes_.find(resource);
  if (mutex_it == mutexes_.end()) {
    return;
  }

  RefCounterMutex& ref_mutex = mutex_it->second;
  if (--ref_mutex.use_count == 0u) {
    mutexes_.erase(mutex_it);
  }
}

NamedMutexStorage::NamedMutexStorage() : impl_(std::make_shared<Impl>()) {}

std::mutex& NamedMutexStorage::AquireLock(const std::string& resource) {
  return impl_->AquireLock(resource);
}

void NamedMutexStorage::ReleaseLock(const std::string& resource) {
  impl_->ReleaseLock(resource);
}

NamedMutex::NamedMutex(NamedMutexStorage& storage, const std::string& name)
    : storage_{storage}, name_{name}, mutex_{storage_.AquireLock(name_)} {}

NamedMutex::~NamedMutex() { storage_.ReleaseLock(name_); }

void NamedMutex::lock() { mutex_.lock(); }

bool NamedMutex::try_lock() { return mutex_.try_lock(); }

void NamedMutex::unlock() { mutex_.unlock(); }

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
