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

#include "NamedMutex.h"

#include <thread>

#include <olp/core/porting/make_unique.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

class NamedMutexStorage::Impl {
 public:
  std::mutex& AcquireLock(const std::string& resource);
  void ReleaseLock(const std::string& resource);
  std::condition_variable& GetLockCondition(const std::string& resource);
  std::mutex& GetLockMutex(const std::string& resource);
  void SetError(const std::string& resource, const client::ApiError& error);
  boost::optional<client::ApiError> GetError(const std::string& resource);

 private:
  struct RefCounterMutex {
    std::mutex mutex;
    uint32_t use_count{0u};
    boost::optional<client::ApiError> optional_error;
    std::condition_variable lock_condition;
    std::mutex lock_mutex;
  };

  std::mutex mutex_;
  std::unordered_map<std::string, RefCounterMutex> mutexes_;
};

std::mutex& NamedMutexStorage::Impl::AcquireLock(const std::string& resource) {
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

std::condition_variable& NamedMutexStorage::Impl::GetLockCondition(
    const std::string& resource) {
  std::lock_guard<std::mutex> lock(mutex_);
  RefCounterMutex& ref_mutex = mutexes_[resource];
  return ref_mutex.lock_condition;
}

std::mutex& NamedMutexStorage::Impl::GetLockMutex(const std::string& resource) {
  std::lock_guard<std::mutex> lock(mutex_);
  RefCounterMutex& ref_mutex = mutexes_[resource];
  return ref_mutex.lock_mutex;
}

void NamedMutexStorage::Impl::SetError(const std::string& resource,
                                       const client::ApiError& error) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto mutex_it = mutexes_.find(resource);
  if (mutex_it != mutexes_.end()) {
    mutex_it->second.optional_error = error;
  }
}

boost::optional<client::ApiError> NamedMutexStorage::Impl::GetError(
    const std::string& resource) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto mutex_it = mutexes_.find(resource);
  if (mutex_it == mutexes_.end()) {
    return boost::none;
  }

  return mutex_it->second.optional_error;
}

NamedMutexStorage::NamedMutexStorage() : impl_(std::make_shared<Impl>()) {}

std::mutex& NamedMutexStorage::AcquireLock(const std::string& resource) {
  return impl_->AcquireLock(resource);
}

void NamedMutexStorage::ReleaseLock(const std::string& resource) {
  impl_->ReleaseLock(resource);
}

std::condition_variable& NamedMutexStorage::GetLockCondition(
    const std::string& resource) {
  return impl_->GetLockCondition(resource);
}

std::mutex& NamedMutexStorage::GetLockMutex(const std::string& resource) {
  return impl_->GetLockMutex(resource);
}

void NamedMutexStorage::SetError(const std::string& resource,
                                 const client::ApiError& error) {
  impl_->SetError(resource, error);
}

boost::optional<client::ApiError> NamedMutexStorage::GetError(
    const std::string& resource) {
  return impl_->GetError(resource);
}

NamedMutex::NamedMutex(NamedMutexStorage& storage, const std::string& name,
                       client::CancellationContext& context)
    : storage_{storage},
      context_{context},
      is_locked_{false},
      name_{name},
      mutex_{storage_.AcquireLock(name_)},
      lock_condition_{storage_.GetLockCondition(name_)},
      lock_mutex_{storage_.GetLockMutex(name_)},
      is_canceled_{false} {}

NamedMutex::~NamedMutex() { storage_.ReleaseLock(name_); }

void NamedMutex::lock() {
  const bool executed = context_.ExecuteOrCancelled([&]() {
    return client::CancellationToken([&]() {
      is_canceled_.store(true);
      Notify();
    });
  });

  is_canceled_.store(!executed);

  if (executed) {
    std::unique_lock<std::mutex> unique_lock{lock_mutex_};
    lock_condition_.wait(unique_lock,
                         [&] { return is_canceled_ || try_lock(); });
  }

  // Reset the cancel token
  context_.ExecuteOrCancelled([]() { return client::CancellationToken(); });
}

bool NamedMutex::try_lock() { return is_locked_ = mutex_.try_lock(); }

void NamedMutex::unlock() {
  if (is_locked_) {
    mutex_.unlock();
    is_locked_ = false;
    Notify();
  }
}

void NamedMutex::SetError(const client::ApiError& error) {
  storage_.SetError(name_, error);
}

boost::optional<client::ApiError> NamedMutex::GetError() {
  return storage_.GetError(name_);
}

void NamedMutex::Notify() {
  // A mutex is required since there is a gap between predicate call and wait
  // call when calling wait in NamedMutex::lock.
  std::unique_lock<std::mutex> unique_lock{lock_mutex_};
  lock_condition_.notify_all();
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
