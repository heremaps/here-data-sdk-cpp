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

#include <array>
#include <limits>

#include <olp/core/math/Vector.h>

namespace olp {
namespace math {
/**
 * @brief Maps an integer type to the one that
 * can handle arithmetic overflows.
 *
 * For integer types less than 64 bits, it maps to a larger type that can
 * handle overflows. The default implementation handles types, including
 * the floating-point ones. Specifications are used to handle integer types.
 *
 * @note For float, it can overflow as well. However, the overflow case
 * is relatively rare compared to the integer types. Also if `T` itself is
 * a 64-bit integer type, it can overflow as well.
 */
template <typename T,
          int IntegerBits = std::is_integral<T>::value ? sizeof(T) * 8 : 0,
          bool IsSigned = std::numeric_limits<T>::is_signed>
struct OverflowTrait {
  /// An alias for the integer type.
  using Type = T;
};

/// A specialization for 8-bit signed integers.
template <typename T>
struct OverflowTrait<T, 8, true> {
  /// An alias for the `int16_t` integer type.
  using Type = int16_t;
};

/// A specialization for 8-bit unsigned integers.
template <typename T>
struct OverflowTrait<T, 8, false> {
  /// An alias for the `uint16_t` integer type.
  using Type = uint16_t;
};

/// A specialization for 16-bit signed integers.
template <typename T>
struct OverflowTrait<T, 16, true> {
  /// An alias for the `int32_t` integer type.
  using Type = int32_t;
};

/// A specialization for 16-bit unsigned integers.
template <typename T>
struct OverflowTrait<T, 16, false> {
  /// An alias for `uint32_t` integer type.
  using Type = uint32_t;
};

/// A specialization for 32-bit signed integers.
template <typename T>
struct OverflowTrait<T, 32, true> {
  /// An alias for the `int64_t` integer type.
  using Type = int64_t;
};

/// A specialization for 32-bit unsigned integers.
template <typename T>
struct OverflowTrait<T, 32, false> {
  /// An alias for the `uint64_t` integer type.
  using Type = uint64_t;
};

/**
 * @brief The aligned bounding-box implementation.
 *
 * @tparam T The real type.
 * @tparam N The box dimensions.
 */
template <typename T, unsigned int N>
class AlignedBox {
 public:
  /// The number of corners for the box.
  static const unsigned int numCorners = 1 << N;
  /// The box dimensions.
  static const unsigned int dimensions = N;

  /// An alias for the box value type.
  using ValueType = T;
  /// An alias for the overflow trait.
  using OverflowType = typename OverflowTrait<T>::Type;
  /// An alias for the vector type of the box.
  using VectorType = Vector<ValueType, N>;
  /// An alias for the overflow vector type.
  using OverflowVectorType = Vector<OverflowType, N>;

  /// An alias for the corner points array type.
  using CornerArrayType =
      typename std::array<VectorType,
                          numCorners>;

  /**
   * @brief Creates an `AlignedBox` instance.
   *
   * An empty box is created.
   */
  AlignedBox();

  /**
   * @brief Creates an `AlignedBox` instance.
   *
   * If any component of `min` is greater than `max`,
   * an empty box is created.
   *
   * @param[in] min The box minimum point.
   * @param[in] max The box maximum point.
   */
  AlignedBox(const VectorType& min, const VectorType& max);

  /**
   * @brief Creates a copy of the `AlignedBox` instance.
   *
   * @tparam OtherT The real type of the box being copied.
   * @param[in] other The box to be copied.
   */
  template <typename OtherT>
  AlignedBox(const AlignedBox<OtherT, N>& other);

  /**
   * @brief Resets the box to empty.
   */
  void Reset();

  /**
   * @brief Resets the box to the new minimum and maximum points.
   *
   * If any component of `min` is greater than `max`,
   * an empty box is created.
   *
   * @param[in] min The box minimum point.
   * @param[in] max The box maximum point.
   */
  void Reset(const VectorType& min, const VectorType& max);

  /**
   * @brief Tests whether the box is empty.
   *
   * @return True if the box is empty; false otherwise.
   */
  bool Empty() const;

  /**
   * @brief Gets the center point of the box.
   *
   * The center of an empty box is undefined.
   *
   * @return The box center point.
   */
  VectorType Center() const;

  /**
   * @brief Gets the size of the box.
   *
   * The size of an empty box is zero.
   *
   * @return The size of each dimension.
   *
   * @note An overflow might happen. For example,
   * when `max` is `INT_MAX` and `min` is `INT_MIN`.
   */
  VectorType Size() const;

  /**
   * @brief Gets the box minimum corner point.
   *
   * The minimum corner point of an empty box is undefined.
   *
   * @return The minimum point of the box.
   */
  const VectorType& Minimum() const;

  /**
   * @brief Gets the box maximum corner point.
   *
   * The maximum corner point of an empty box is undefined.
   *
   * @return The maximum point of the box.
   */
  const VectorType& Maximum() const;

  /**
   * @brief Gets the corner points of the box.
   *
   * The corner points of an empty box are undefined.
   *
   * @return An array containing the box corner points.
   */
  CornerArrayType Corners() const;
  /**
   * @brief Tests whether the box contains a point.
   *
   * This test is inclusive.
   * 
   * @param point The point to be tested.
   * @param epsilon The epsilon around the point.
   *
   * @return True if the box contains the point; false otherwise.
   */
  bool Contains(const VectorType& point, T epsilon = T()) const;

  /**
   * @brief Tests whether the box contains another box.
   *
   * @param[in] box The aligned box.
   *
   * @return True if the box contains another box; false otherwise.
   */
  bool Contains(const AlignedBox& box) const;

  /**
   * @brief Tests whether the box intersects another box.
   *
   * A test box is considered to be intersecting if 
   * another box contains it.
   *
   * @param[in] box The box to be tested.
   *
   * @return True if the boxes intersect; false otherwise.
   */
  bool Intersects(const AlignedBox& box) const;

  /**
   * @brief Computes the nearest point on the box to a point.
   *
   * The nearest point to an empty box is undefined.
   *
   * @param[in] point The point from which to find the nearest point.
   *
   * @return The nearest point on the box to the specified point.
   */
  VectorType NearestPoint(const VectorType& point) const;

  /**
   * @brief Computes the distance from a point to the box.
   *
   * A point on or inside the box has a distance of zero.
   *
   * The distance to an empty box is undefined.
   *
   * @param[in] point The point from which to find the distance.
   *
   * @return The distance between the point and the box.
   */
  ValueType Distance(const VectorType& point) const;

  /**
   * @brief Computes the squared distance from a point to the box.
   *
   * A point on or inside the box has a squared distance of zero.
   *
   * The squared distance to an empty box is undefined.
   *
   * @param[in] point The point from which to find the squared distance.
   *
   * @return The squared distance between the point and the box.
   */
  ValueType Distance2(const VectorType& point) const;

  /**
   * @brief Checks whether two boxes are equal.
   *
   * @param[in] box The other box.
   */
  bool operator==(const AlignedBox& box) const;

  /**
   * @brief Checks whether two boxes are not equal.
   *
   * @param[in] box The other box.
   */
  bool operator!=(const AlignedBox& box) const;

 private:
  template <typename U, unsigned int X>
  friend class AlignedBox;

  /// The box minimum point.
  VectorType minimum_;
  /// The box maximum point.
  VectorType maximum_;
};

#ifndef NDEBUG
template <typename T>
bool CheckAddOperationOverflow(T a, T b) {
  return ((a > 0) && (b > 0) && (a > (std::numeric_limits<T>::max() - b))) ||
         ((a < 0) && (b < 0) && (a < (std::numeric_limits<T>::min() - b)));
}
#endif

/// The 3D double-precision box type.
using AlignedBox3d =
    AlignedBox<double, 3>;

template <typename T, unsigned int N>
AlignedBox<T, N>::AlignedBox()
    : minimum_(std::numeric_limits<T>::max()),
      maximum_(std::numeric_limits<T>::lowest()) {}

template <typename T, unsigned int N>
template <typename OtherT>
AlignedBox<T, N>::AlignedBox(const AlignedBox<OtherT, N>& other)
    : minimum_(other.min()), maximum_(other.max()) {}

template <typename T, unsigned int N>
AlignedBox<T, N>::AlignedBox(const VectorType& min, const VectorType& max)
    : minimum_(min), maximum_(max) {}

template <typename T, unsigned int N>
void AlignedBox<T, N>::Reset() {
  minimum_ = VectorType(std::numeric_limits<T>::max());
  maximum_ = VectorType(std::numeric_limits<T>::lowest());
}

template <typename T, unsigned int N>
void AlignedBox<T, N>::Reset(const VectorType& min, const VectorType& max) {
  minimum_ = min;
  maximum_ = max;
}

template <typename T, unsigned int N>
typename AlignedBox<T, N>::VectorType AlignedBox<T, N>::Center() const {
#ifndef NDEBUG
  // Checks for possible 64-bit integer overflows or underflows.
  if (std::numeric_limits<OverflowType>::is_integer &&
      sizeof(OverflowType) * 8 == 64) {
    for (unsigned dim = 0; dim < dimensions; ++dim) {
      if (CheckAddOperationOverflow<OverflowType>(maximum_[dim],
                                                  minimum_[dim])) {
        // DEBUG_ASSERT( false );
        return VectorType(0);
      }
    }
  }
#endif

  return VectorType(
      (OverflowVectorType(maximum_) + OverflowVectorType(minimum_)) /
      OverflowVectorType(2));
}

template <typename T, unsigned int N>
bool AlignedBox<T, N>::Empty() const {
  Vector3<bool> result = maximum_.LessThan(minimum_);
  return result.x || result.y || result.z;
}

template <typename T, unsigned int N>
typename AlignedBox<T, N>::VectorType AlignedBox<T, N>::Size() const {
  return Empty() ? VectorType(0) : maximum_ - minimum_;
}

template <typename T, unsigned int N>
const typename AlignedBox<T, N>::VectorType& AlignedBox<T, N>::Minimum() const {
  return minimum_;
}

template <typename T, unsigned int N>
const typename AlignedBox<T, N>::VectorType& AlignedBox<T, N>::Maximum() const {
  return maximum_;
}

template <typename T, unsigned int N>
bool AlignedBox<T, N>::Contains(const VectorType& point, T epsilon) const {
  if (Empty()) {
    return false;
  }

  for (unsigned dim = 0; dim < dimensions; dim++) {
    if (minimum_[dim] - epsilon > point[dim] ||
        maximum_[dim] + epsilon < point[dim])
      return false;
  }

  return true;
}

template <typename T, unsigned int N>
bool AlignedBox<T, N>::Contains(const AlignedBox& box) const {
  if (Empty()) {
    return false;
  }

  for (unsigned dim = 0; dim < dimensions; ++dim) {
    if (box.minimum_[dim] < minimum_[dim] ||
        box.maximum_[dim] > maximum_[dim]) {
      return false;
    }
  }

  return true;
}

template <typename T, unsigned int N>
bool AlignedBox<T, N>::Intersects(const AlignedBox& box) const {
  if (Empty()) {
    return false;
  }

  const VectorType& otherMin = box.min();
  const VectorType& otherMax = box.max();
  for (unsigned dim = 0; dim < dimensions; dim++) {
    if ((maximum_[dim] < otherMin[dim]) || (minimum_[dim] > otherMax[dim]))
      return false;
  }

  return true;
}

template <typename T, unsigned int N>
auto AlignedBox<T, N>::Distance(const VectorType& point) const -> ValueType {
  return static_cast<ValueType>(sqrt(distance2(point)));
}

template <typename T, unsigned int N>
auto AlignedBox<T, N>::Distance2(const VectorType& point) const -> ValueType {
  T distance2 = 0;
  for (unsigned dim = 0; dim < dimensions; dim++) {
    if (point[dim] < minimum_[dim]) {
      T dimDistance = minimum_[dim] - point[dim];
      distance2 += dimDistance * dimDistance;
    } else if (point[dim] > maximum_[dim]) {
      T dimDistance = point[dim] - maximum_[dim];
      distance2 += dimDistance * dimDistance;
    }
  }

  return distance2;
}

template <typename T, unsigned int N>
bool AlignedBox<T, N>::operator==(const AlignedBox& box) const {
  //
  // If either box is empty, checks which ones are
  // empty.
  //
  bool thisEmpty = Empty();
  bool otherEmpty = box.empty();
  if (thisEmpty || otherEmpty) {
    return thisEmpty == otherEmpty;
  }

  return minimum_ == box.min() && maximum_ == box.max();
}

template <typename T, unsigned int N>
bool AlignedBox<T, N>::operator!=(const AlignedBox& box) const {
  return !(*this == box);
}

}  // namespace math
}  // namespace olp
