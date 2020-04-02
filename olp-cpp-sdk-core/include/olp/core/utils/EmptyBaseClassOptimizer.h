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

#include <type_traits>
#include <utility>

namespace olp {
namespace utils {
/*
 * Exposes the class T via data( ) function
 * The user is supposed to derive from EmptyBaseClassOptimizer to benefit from
 * the compiler's Empty Base Class Optimization This class makes it possible to
 * use this technique even if one does not know whether one receives a struct or
 * an internal data type (from which one normally cannot derive) E.g. struct
 * NoData { }; struct MyStruct : EmptyBaseClassOptimizer< NoData >
 * {
 *     int x;
 * }
 * sizeof( MyStruct ) == sizeof( int );
 */
template <typename T, typename Enable = void>
struct EmptyBaseClassOptimizer;

template <typename T>
struct EmptyBaseClassOptimizer<
    T, typename std::enable_if<std::is_class<T>::value, void>::type>
    : private T {
  template <typename... Args>
  explicit EmptyBaseClassOptimizer(Args&&... args)
      : T(std::forward<Args>(args)...) {}

  T& data() { return *this; }

  const T& data() const { return *this; }
};

template <typename T>
struct EmptyBaseClassOptimizer<
    T, typename std::enable_if<!std::is_class<T>::value, void>::type> {
  EmptyBaseClassOptimizer() : data_() {}

  explicit EmptyBaseClassOptimizer(T data) : data_(data) {}

  T& data() { return data_; }

  const T& data() const { return data_; }

 private:
  T data_;
};
}  // namespace utils
}  // namespace olp
