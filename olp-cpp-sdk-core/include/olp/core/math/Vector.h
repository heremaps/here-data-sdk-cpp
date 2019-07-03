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

namespace olp {
namespace math {

template <typename T>
struct Vector2 {
  Vector2() {}
  Vector2(T a, T b) {
    x = a;
    y = b;
  }

  Vector2<T>& operator=(Vector2<T> const& v) {
    this->x = static_cast<T>(v.x);
    this->y = static_cast<T>(v.y);
    return *this;
  }

  Vector2<T> operator*(T const& s) const { return Vector2<T>(x * s, y * s); }
  Vector2<T> operator-() { return Vector2<T>(-x, -y); }

  Vector2<T> operator-(Vector2<T> const& v) const {
    return Vector2<T>(x - v.x, y - v.y);
  }

  Vector2<T> operator+(Vector2<T> const& v) const {
    return Vector2<T>(x + v.x, y + v.y);
  }

  T x;
  T y;
};

template <typename T>
struct Vector3 {
  Vector3() {}
  Vector3(T a, T b, T c) {
    x = a;
    y = b;
    z = c;
  }
  Vector3(T const& s) : x(s), y(s), z(s) {}

  Vector3<T> operator*(T const& s) const {
    return Vector3<T>(x * s, y * s, z * s);
  }
  Vector3<T> operator-() const { return Vector3<T>(-x, -y, -z); }

  Vector3<T> operator-(Vector3<T> const& v) const {
    return Vector3<T>(x - v.x, y - v.y, z - v.z);
  }

  Vector3<T> operator+(Vector3<T> const& v) const {
    return Vector3<T>(x + v.x, y + v.y, z + v.z);
  }

  Vector3<bool> LessThan(Vector3<T> const& v) const {
    return Vector3<bool>(x < v.x, y < v.y, z < v.z);
  }

  T x;
  T y;
  T z;
};

template <typename T, unsigned int N>
struct VectorImpl;

template <typename T>
struct VectorImpl<T, 2> {
  using type = Vector2<T>;
};

template <typename T>
struct VectorImpl<T, 3> {
  using type = Vector3<T>;
};

template <typename T, unsigned int N>
using Vector = typename VectorImpl<T, N>::type;

}  // namespace math
}  // namespace olp
