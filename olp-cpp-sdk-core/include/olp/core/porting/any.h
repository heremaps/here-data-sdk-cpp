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

#if (__cplusplus >= 201703L) && defined(OLP_SDK_USE_STD_ANY)
#include <any>

namespace olp {
namespace porting {

using any = std::any;

template <typename T>
inline T any_cast(const any& operand) {
  return std::any_cast<T>(operand);
}

template <typename T>
inline T any_cast(any& operand) {
  return std::any_cast<T>(operand);
}

template <typename T>
inline T any_cast(any&& operand) {
  return std::any_cast<T>(std::move(operand));
}

template <typename T>
inline const T* any_cast(const any* operand) noexcept {
  return std::any_cast<T>(operand);
}

template <typename T>
inline T* any_cast(any* operand) noexcept {
  return std::any_cast<T>(operand);
}

inline bool has_value(const any& operand) noexcept {
  return operand.has_value();
}

inline void reset(any& operand) noexcept { operand.reset(); }

template <typename T, typename... Args>
inline any make_any(Args&&... args) {
  return std::make_any<T>(std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
inline any make_any(std::initializer_list<U> il, Args&&... args) {
  return std::make_any<T>(il, std::forward<Args>(args)...);
}

}  // namespace porting
}  // namespace olp

#else
#include <boost/any.hpp>

namespace olp {
namespace porting {

using any = boost::any;

template <typename T>
inline T any_cast(const any& operand) {
  return boost::any_cast<T>(operand);
}

template <typename T>
inline T any_cast(any& operand) {
  return boost::any_cast<T>(operand);
}

template <typename T>
inline T any_cast(any&& operand) {
  return boost::any_cast<T>(operand);
}

template <typename T>
inline const T* any_cast(const any* operand) noexcept {
  return boost::any_cast<T>(operand);
}

template <typename T>
inline T* any_cast(any* operand) noexcept {
  return boost::any_cast<T>(operand);
}

inline bool has_value(const any& operand) noexcept { return !operand.empty(); }

inline void reset(any& operand) noexcept { operand = boost::any(); }

template <typename T, typename... Args>
inline any make_any(Args&&... args) {
  return any(T(std::forward<Args>(args)...));
}

template <typename T, typename U, typename... Args>
inline any make_any(std::initializer_list<U> il, Args&&... args) {
  return any(T(il, std::forward<Args>(args)...));
}

}  // namespace porting
}  // namespace olp

#endif
