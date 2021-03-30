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

#include <olp/core/math/Vector.h>

namespace olp {
namespace math {
/// Represents the 2D size.
template <typename T>
class Size2 {
 public:
  /// An alias for the value type.
  using Value = T;

  /// Creates an uninitialized `Size2` instance.
  Size2();

  /**
   * @brief Creates a `Size2` instance from another type.
   *
   * @param size The size of the other type to create from.
   */
  template <typename U>
  Size2(const Size2<U>& size);

  /**
   * @brief Creates a `Size2` instance from a vector.
   *
   * @param[in] vector The vector to create from.
   */
  template <typename U>
  explicit Size2(const Vector2<U>& vector);

  /**
   * @brief Creates a `Size2` instance from the value width and height.
   *
   * @param[in] width The value width to create from.
   * @param[in] height The value height to create from.
   */
  Size2(Value width, Value height);

  /**
   * @brief Converts a type to a vector.
   *
   * @return The vector.
   */
  template <typename U>
  explicit operator Vector2<U>() const;

  /**
   * @brief Checks whether the size is empty.
   *
   * @return True if any of the size components is zero;
   * false otherwise.
   */
  bool empty() const;

  /**
   * @brief Gets the width of the `Size2` object.
   *
   * @return The width of the `Size2` object.
   */
  Value Width() const;

  /**
   * @brief Gets the height of the `Size2` object.
   *
   * @return The height of the `Size2` object.
   */
  Value Height() const;

 private:
  Vector2<Value> size_;
};

using Size2u = Size2<unsigned>;

template <typename T>
Size2<T>::Size2() : size_(0, 0) {}

template <typename T>
template <typename U>
Size2<T>::Size2(const Size2<U>& size) : size_(size.width(), size.height()) {}

template <typename T>
template <typename U>
Size2<T>::Size2(const Vector2<U>& vector) : size_(vector) {}

template <typename T>
Size2<T>::Size2(Value width, Value height) : size_(width, height) {}

template <typename T>
template <typename U>
Size2<T>::operator Vector2<U>() const {
  return Vector2<U>(size_.x, size_.y);
}

template <typename T>
bool Size2<T>::empty() const {
  return size_.x == T() || size_.y == T();
}

template <typename T>
auto Size2<T>::Width() const -> Value {
  return size_.x;
}

template <typename T>
auto Size2<T>::Height() const -> Value {
  return size_.y;
}

template <typename T>
bool operator==(const Size2<T>& lhs, const Size2<T>& rhs) {
  return lhs.width() == rhs.width() && lhs.height() == rhs.height();
}

template <typename T>
bool operator!=(const Size2<T>& lhs, const Size2<T>& rhs) {
  return !(lhs == rhs);
}

}  // namespace math
}  // namespace olp
