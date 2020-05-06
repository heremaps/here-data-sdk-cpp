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
#include <utility>

namespace olp {
namespace thread {
/**
 * @brief A simple atomic wrapper.
 *
 * Depending on the provided `MutexType`, it can be
 * multi-read/single-write or single-read/single-write.
 *
 * @tparam Type The member type to which atomic access is required.
 * @tparam MutexType Defines the lock type.
 * @tparam ReadLockType Defines the locking strategy for the atomic read
 * operation.
 */
template <class Type, typename MutexType = std::mutex,
          typename ReadLockType = std::lock_guard<MutexType> >
class Atomic {
 public:
  /**
   * @brief Alias for the type.
   */
  using value_type = Type;

  /**
   * @brief Alias for the atomic write mutex lock.
   */
  using WriteLockType = std::lock_guard<MutexType>;

  /**
   * @brief Creates the `Atomic` instance.
   *
   * @tparam SomeType The variable type that is used for initialization.
   * @param am The rvalue reference of the `SomeType` instance.
   */
  template <class SomeType>
  explicit Atomic(SomeType&& am) : m(std::forward<SomeType>(am)) {}

  /**
   * @brief Creates the `Atomic` instance.
   */
  Atomic() : m() {}

  /**
   * @brief Calls the lambda function using the unique lock.
   *
   * @tparam Functor Accepts the single parameter of `Type&`.
   * @param lambda The function of the `Functor` type.
   *
   * @return The lambda result.
   */
  template <class Functor>
  auto locked(Functor&& lambda) -> decltype(lambda(std::declval<Type&>())) {
    WriteLockType lock(m_mutex);
    return lambda(m);
  }

  /**
   * @brief Calls the lambda using the lock.
   *
   * Used for the const version of this class.
   *
   * @tparam Functor Accepts the single parameter of const `Type&`.
   * @param lambda The function of the `Functor` type.
   *
   * @return The lambda result.
   */
  template <class Functor>
  auto locked(Functor&& lambda) const
      -> decltype(lambda(std::declval<const Type&>())) const {
    ReadLockType lock(m_mutex);
    return lambda(m);
  }

  /**
   * @brief Gets a copy of the data using the lock.
   *
   * @return The copy of the data.
   */
  Type lockedCopy() const {
    ReadLockType lock(m_mutex);
    return m;
  }

  /**
   * @brief Gets a copy of the moved data using the unique lock.
   *
   * @return The copy of the moved data.
   */
  Type lockedMove() {
    WriteLockType lock(m_mutex);
    return std::move(m);
  }

  /**
   * @brief Assigns the data using the unique lock.
   *
   * @tparam SomeType The variable type that is used for initialization.
   * @param am The rvalue reference of the `SomeType` instance.
   */
  template <class SomeType>
  void lockedAssign(SomeType&& am) {
    WriteLockType lock(m_mutex);
    m = std::forward<SomeType>(am);
  }

  /**
   * @brief Exchanges context with the `Type` object using the unique lock.
   *
   * @param other The copy of the value specified in the `Type` object.
   */
  void lockedSwap(Type& other) {
    WriteLockType lock(m_mutex);
    std::swap(m, other);
  }

  /**
   * @brief Exchanges context with the `Type` object using the unique lock and
   * moves data.
   *
   * @return The copy of the value specified in the `Type` object.
   */
  Type lockedSwapWithDefault() {
    WriteLockType lock(m_mutex);
    Type other{};
    std::swap(m, other);
    return other;
  }

  /**
   * @brief Converts to bool if the wrapped type is bool convertible.
   *
   * @return True if the wrapped type is bool convertible; false otherwise.
   */
  explicit operator bool() const {
    ReadLockType lock(m_mutex);
    return static_cast<bool>(m);
  }

 protected:
  /**
   * @brief Defines the lock type.
   */
  mutable MutexType m_mutex;

  /**
   * @brief The member type to which atomic access is required.
   */
  Type m;
};

}  // namespace thread
}  // namespace olp
