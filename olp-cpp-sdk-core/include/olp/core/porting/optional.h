/*
 * Copyright (C) 2025 HERE Europe B.V.
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

#if (__cplusplus >= 201703L) && defined(OLP_CPP_SDK_USE_STD_OPTIONAL)
#include <optional>

namespace olp {
namespace porting {

template <typename T>
using optional = std::optional<T>;
constexpr auto none = std::nullopt;

template <typename T, typename U = T>
constexpr auto value_or(const optional<T>& opt, U&& def) {
  return opt.value_or(std::forward<U>(def));
}

template <typename T>
constexpr const T* get_ptr(const optional<T>& opt) {
  return opt ? &*opt : nullptr;
}

template <typename T>
constexpr T* get_ptr(optional<T>& opt) {
  return opt ? &*opt : nullptr;
}

template <typename T>
optional<std::decay_t<T>> make_optional(T&& v) {
  return optional<std::decay_t<T>>(std::forward<T>(v));
}

}  // namespace porting
}  // namespace olp

#else
#include <boost/optional.hpp>

namespace olp {
namespace porting {

template <typename T>
using optional = boost::optional<T>;

constexpr auto none = boost::none;

template <typename T>
constexpr typename optional<T>::reference_const_type value_or(
    const optional<T>& opt, typename optional<T>::reference_const_type def) {
  return opt.get_value_or(def);
}

template <typename T>
constexpr typename optional<T>::reference_type value_or(
    optional<T>& opt, typename optional<T>::reference_type def) {
  return opt.get_value_or(def);
}

template <typename T>
constexpr typename optional<T>::pointer_const_type get_ptr(
    const optional<T>& opt) {
  return opt.get_ptr();
}

template <typename T>
constexpr typename optional<T>::pointer_type get_ptr(optional<T>& opt) {
  return opt.get_ptr();
}

template <typename T>
optional<BOOST_DEDUCED_TYPENAME boost::decay<T>::type> make_optional(T&& v) {
  return optional<BOOST_DEDUCED_TYPENAME boost::decay<T>::type>(
      boost::forward<T>(v));
}

}  // namespace porting
}  // namespace olp

#endif
