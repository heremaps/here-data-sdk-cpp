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

#include <memory>

#if ((__cplusplus >= 201304L) || (defined(_MSC_VER) && _MSC_VER >= 1800))
#define HAVE_STD_MAKE_UNIQUE
#endif

#if !defined(HAVE_STD_MAKE_UNIQUE)

#include <cstdint>
#include <type_traits>
#include <utility>

// Implementation is taken from here https://isocpp.org/files/papers/N3656.txt

namespace std {
namespace {
template <class T>
struct unique_if {
  typedef unique_ptr<T> single_object;
};

template <class T>
struct unique_if<T[]> {
  typedef unique_ptr<T[]> unknown_bound;
};

template <class T, size_t N>
struct unique_if<T[N]> {
  typedef void known_bound;
};
}  // namespace

/**
 * Construct an object from arguments and return unique_ptr that manages
 * ownership of it.
 * @param[in] args Variadic arguments that are passed to constructor of the
 * object.
 * @return unique_ptr that manages ownership of the object created.
 */
template <class T, class... Args>
typename unique_if<T>::single_object make_unique(Args&&... args) {
  return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/**
 * Construct an array of objects and return unique_ptr that manages ownership of
 * it.
 * @param[in] n Array extent.
 * @return unique_ptr that manages ownership of the array reated.
 */
template <class T>
typename unique_if<T>::unknown_bound make_unique(size_t n) {
  typedef typename remove_extent<T>::type U;
  return unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename unique_if<T>::known_bound make_unique(Args&&...) = delete;

}  // namespace std

#endif  // HAVE_STD_MAKE_UNIQUE
