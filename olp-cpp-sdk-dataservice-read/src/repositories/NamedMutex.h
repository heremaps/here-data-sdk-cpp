/*
 * Copyright (C) 2020-2022 HERE Europe B.V.
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

#include <olp/core/client/ApiError.h>
#include <olp/core/client/CancellationContext.h>
#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

/*
 * @brief A mutex storage class, used to store and access mutex primitives by
 * name, so it can be used in different places and different conditions. Has an
 * ability to share an `ApiError` among threads that uses the same mutex.
 */
class NamedMutexStorage {
 public:
  NamedMutexStorage();

  std::mutex& AquireLock(const std::string& resource);
  void ReleaseLock(const std::string& resource);

  /**
   * @brief Saves an error to share it among threads.
   *
   * @param resource A name of a mutex to store an error for.
   * @param error An error to be stored.
   */
  void SetError(const std::string& resource, const client::ApiError& error);

  /**
   * @brief Gets the stored error for provided resource.
   *
   * @param resource A name of a mutex to get an error for.
   *
   * @return The stored `ApiError` instance or `boost::none` if no error has
   * been stored for this resource.
   */
  boost::optional<client::ApiError> GetError(const std::string& resource);

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

/*
 * @brief A synchronization primitive that can be used to protect shared data
 * from being simultaneously accessed by multiple threads. Has an ability to
 * share an `ApiError` among the threads.
 */
class NamedMutex final {
 public:
  NamedMutex(NamedMutexStorage& storage, const std::string& name,
             const client::CancellationContext& context);

  NamedMutex(const NamedMutex&) = delete;
  NamedMutex(NamedMutex&&) = delete;
  NamedMutex& operator=(const NamedMutex&) = delete;
  NamedMutex& operator=(NamedMutex&&) = delete;

  ~NamedMutex();

  // These method names are lower snake case to be able to use NamedMutex with
  // std::unique_lock.
  void lock();
  bool try_lock();
  void unlock();

  /**
   * @brief Saves an error to share it among threads.
   *
   * @param error An error to be stored.
   */
  void SetError(const client::ApiError& error);

  /**
   * @brief Gets the stored error for this mutex.
   *
   * @return The stored `ApiError` instance  or `boost::none` if no error has
   * been stored for this mutex.
   */
  boost::optional<client::ApiError> GetError();

 private:
  NamedMutexStorage& storage_;
  const client::CancellationContext& context_;
  bool is_locked_;
  std::string name_;
  std::mutex& mutex_;
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
