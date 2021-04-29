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

#include <memory>
#include <unordered_map>

#include <olp/core/porting/make_unique.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {

static std::mutex gMutex;

struct RefCounterMutex {
  explicit RefCounterMutex() : mutex(std::make_unique<std::mutex>()) {}

  std::unique_ptr<std::mutex> mutex;
  uint32_t use_count{0};
};

static std::unordered_map<std::string, RefCounterMutex> gMutexes;

std::mutex& AquireLock(const std::string& resource) {
  std::unique_lock<std::mutex> lock(gMutex);
  RefCounterMutex& ref_mutex = gMutexes[resource];
  ref_mutex.use_count++;
  return *ref_mutex.mutex;
}

void ReleaseLock(const std::string& resource) {
  std::unique_lock<std::mutex> lock(gMutex);
  RefCounterMutex& ref_mutex = gMutexes[resource];
  if (--ref_mutex.use_count == 0) {
    gMutexes.erase(resource);
  }
}

}  // namespace

NamedMutex::NamedMutex(const std::string& name)
    : name_{name}, mutex_{AquireLock(name_)} {}

NamedMutex::~NamedMutex() { ReleaseLock(name_); }

void NamedMutex::lock() { mutex_.lock(); }

bool NamedMutex::try_lock() { return mutex_.try_lock(); }

void NamedMutex::unlock() { mutex_.unlock(); }

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
