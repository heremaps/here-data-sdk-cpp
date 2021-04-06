/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

namespace olp {
namespace math {

template <typename T>
/// Represents 2D vectors and points.
struct Vector2 {
  Vector2() {}
  /**
   * @brief Creates a `Vector2` instance with the given X and Y components.
   * 
   * @param a The `X` component of the vector.
   * @param b The `Y` component of the vector.
   */
  Vector2(T a, T b) {
    x = a;
    y = b;
  }

  /**
   * @brief Assigns components of one vector to another vector.
   * 
   * @param v The vector to take components from.
   */
  Vector2<T>& operator=(Vector2<T> const& v) {
    this->x = static_cast<T>(v.x);
    this->y = static_cast<T>(v.y);
    return *this;
  }

  /**
   * @brief Multiplies each vector component by a number.
   * 
   * @param s The number to multiply by.
   */
  Vector2<T> operator*(T const& s) const { return Vector2<T>(x * s, y * s); }
  /// Negates the vector.
  Vector2<T> operator-() { return Vector2<T>(-x, -y); }

  /**
   * @brief Subtracts one vector from another.
   * 
   * @param v The vector to substruct from.
   */
  Vector2<T> operator-(Vector2<T> const& v) const {
    return Vector2<T>(x - v.x, y - v.y);
  }

  /**
   * @brief Adds corresponding components of two vectors.
   * 
   * @param v The vector to add.
   */
  Vector2<T> operator+(Vector2<T> const& v) const {
    return Vector2<T>(x + v.x, y + v.y);
  }

  /// The `X` component of the vector.
  T x;
  /// The `Y` component of the vector.
  T y;
};

template <typename T>
/// Represents 3D vectors and points.
struct Vector3 {
  Vector3() {}
  /**
   * @brief Creates a `Vector3` instance with the given X, Y, and Z components.
   * 
   * @param a The `X` component of the vector.
   * @param b The `Y` component of the vector.
   * @param c The `Z` component of the vector.
   */
  Vector3(T a, T b, T c) {
    x = a;
    y = b;
    z = c;
  }
  /**
   * @brief Creates a `Vector3` instance.
   * 
   * @param s The constant to initialize the vector components.
   */
  explicit Vector3(T const& s) : x(s), y(s), z(s) {}

  /**
   * @brief Multiplies each vector component by a number.
   * 
   * @param s The number to multiply by.
   */
  Vector3<T> operator*(T const& s) const {
    return Vector3<T>(x * s, y * s, z * s);
  }
  ///@brief Negates the vector.
  Vector3<T> operator-() const { return Vector3<T>(-x, -y, -z); }

  /**
   * @brief Subtracts one vector from another.
   * 
   * @param v The vector to substruct from. 
   */
  Vector3<T> operator-(Vector3<T> const& v) const {
    return Vector3<T>(x - v.x, y - v.y, z - v.z);
  }

  /**
   * @brief Adds corresponding components of two vectors.
   * 
   * @param v The vector to add.
   */
  Vector3<T> operator+(Vector3<T> const& v) const {
    return Vector3<T>(x + v.x, y + v.y, z + v.z);
  }

  /**
   * @brief Checks whether the parameters of one vector
   * are less than the parameters of the other vector.
   * 
   * @param v The vector to compare to.
   */
  Vector3<bool> LessThan(Vector3<T> const& v) const {
    return Vector3<bool>(x < v.x, y < v.y, z < v.z);
  }

  /// The `X` component of the vector.
  T x;
  /// The `Y` component of the vector.
  T y;
  /// The `Z` component of the vector.
  T z;
};

template <typename T, unsigned int N>
/// The implementation structure of a vector.
struct VectorImpl;

template <typename T>
/// The implementation structure of a 2D vector.
struct VectorImpl<T, 2> {
  /// An alias for the 2D vector.
  using type = Vector2<T>;
};

template <typename T>
/// The implementation structure of a 3D vector.
struct VectorImpl<T, 3> {
  /// An alias for the 3D vector.
  using type = Vector3<T>;
};

template <typename T, unsigned int N>
using Vector = typename VectorImpl<T, N>::type;

}  // namespace math
}  // namespace olp
