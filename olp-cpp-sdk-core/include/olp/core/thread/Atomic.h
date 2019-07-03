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

#include <functional>
#include <mutex>

namespace olp {
namespace thread {
/**
 * Simple atomic wrapper.
 * Depending on the provided MutexType this can be either multi-read /
 * single-write or single-read / single-write.
 */
template <class Type, typename MutexType = std::mutex,
          typename ReadLockType = std::lock_guard<MutexType> >
class Atomic {
 public:
  using value_type = Type;
  using WriteLockType = std::lock_guard<MutexType>;

  /**
   * Construct.
   */
  template <class SomeType>
  Atomic(SomeType&& am) : m(std::forward<SomeType>(am)) {}

  /**
   * Construct.
   */
  Atomic() : m() {}

  /**
   * Calls the lambda while having a unique lock on the data.
   */
  template <class Functor>
  auto locked(Functor&& lambda) -> decltype(lambda(std::declval<Type&>())) {
    WriteLockType lock(m_mutex);
    return lambda(m);
  }

  /**
   * Calls the lambda while having a lock on the data.
   * const version.
   */
  template <class Functor>
  auto locked(Functor&& lambda) const
      -> decltype(lambda(std::declval<const Type&>())) const {
    ReadLockType lock(m_mutex);
    return lambda(m);
  }

  /**
   * Return a copy of the data while holding a lock.
   */
  Type lockedCopy() const {
    ReadLockType lock(m_mutex);
    return m;
  }

  /**
   * Return the moved data while having a unique lock.
   */
  Type lockedMove() {
    WriteLockType lock(m_mutex);
    return std::move(m);
  }

  /**
   * Assign the data while having a unique lock.
   */
  template <class SomeType>
  void lockedAssign(SomeType&& am) {
    WriteLockType lock(m_mutex);
    m = std::forward<SomeType>(am);
  }

  /**
   * Swap while having a unique lock on the data.
   */
  void lockedSwap(Type& other) {
    WriteLockType lock(m_mutex);
    std::swap(m, other);
  }

  /**
   * Swap with Type() while having a unique lock on the data and move.
   */
  Type lockedSwapWithDefault() {
    WriteLockType lock(m_mutex);
    Type other{};
    std::swap(m, other);
    return other;
  }

  /**
   * Convert to bool if wrapped type is bool convertible.
   */
  explicit operator bool() const {
    ReadLockType lock(m_mutex);
    return static_cast<bool>(m);
  }

 protected:
  mutable MutexType m_mutex;
  Type m;
};

}  // namespace thread
}  // namespace olp
