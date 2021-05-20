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

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

/*
 * @brief A mutex storage class, used to store and access mutex primitives by
 * name, so it can be used in different places and different conditions.
 */
class NamedMutexStorage {
 public:
  NamedMutexStorage();

  std::mutex& AquireLock(const std::string& resource);
  void ReleaseLock(const std::string& resource);

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

/*
 * @brief A synchronization primitive that can be used to protect shared data
 * from being simultaneously accessed by multiple threads.
 */
class NamedMutex final {
 public:
  NamedMutex(NamedMutexStorage& storage, const std::string& name);

  NamedMutex(const NamedMutex&) = delete;
  NamedMutex(NamedMutex&&) = delete;
  NamedMutex& operator=(const NamedMutex&) = delete;
  NamedMutex& operator=(NamedMutex&&) = delete;

  ~NamedMutex();

  void lock();
  bool try_lock();
  void unlock();

 private:
  NamedMutexStorage& storage_;
  std::string name_;
  std::mutex& mutex_;
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
