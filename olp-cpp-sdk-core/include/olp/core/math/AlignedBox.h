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

#include <array>
#include <limits>

#include <olp/core/math/Vector.h>

namespace olp {
namespace math {
/**
 * This type trait tries to map a type to another one which can handle
 * arithmetic overflows.
 *
 * For integer types less than 64 bits, it maps to a larger type which can
 * handle overflows. The default implementation handles types including
 * floating-point ones. Specialization is used to handle integer types.
 * @note for float, it can overflow as well. However, the overflow case in
 * reality is relatively rare compared to the integer types. Also if T itself is
 * 64-bit integer type, it can overflow as well.
 */
template <typename T,
          int IntegerBits = std::is_integral<T>::value ? sizeof(T) * 8 : 0,
          bool IsSigned = std::numeric_limits<T>::is_signed>
struct OverflowTrait {
  using Type = T;
};

/** Specialization for 8-bit signed integers */
template <typename T>
struct OverflowTrait<T, 8, true> {
  using Type = int16_t;
};

/** Specialization for 8-bit unsigned integers */
template <typename T>
struct OverflowTrait<T, 8, false> {
  using Type = uint16_t;
};

/** Specialization for 16-bit signed integers */
template <typename T>
struct OverflowTrait<T, 16, true> {
  using Type = int32_t;
};

/** Specialization for 16-bit unsigned integers */
template <typename T>
struct OverflowTrait<T, 16, false> {
  using Type = uint32_t;
};

/** Specialization for 32-bit signed integers */
template <typename T>
struct OverflowTrait<T, 32, true> {
  using Type = int64_t;
};

/** Specialization for 32-bit unsigned integers */
template <typename T>
struct OverflowTrait<T, 32, false> {
  using Type = uint64_t;
};

/**
 * An aligned bounding-box implementation.
 *
 * @tparam T - The real type.
 * @tparam N - The box dimensions.
 */
template <typename T, unsigned int N>
class AlignedBox {
 private:
 public:
  static const unsigned int numCorners =
      1 << N; /*!< Number of corners for the box. */
  static const unsigned int dimensions = N; /*!< Box dimensions. */

  using ValueType = T; /*!< The box value type. */
  using OverflowType = typename OverflowTrait<T>::Type;
  using VectorType = Vector<ValueType, N>; /*!< VectorType of the box. */
  using OverflowVectorType = Vector<OverflowType, N>;

  using CornerArrayType =
      typename std::array<VectorType,
                          numCorners>; /*!< Corner points array type. */

  /**
   * Default constructor.
   *
   * An empty box is constructed.
   */
  AlignedBox();

  /**
   * @brief Constructor.
   *
   * If the any components of min are greater than max an
   * empty box will result.
   *
   * @param[in] min - The box minimum point.
   * @param[in] max - The box maximum point.
   */
  AlignedBox(const VectorType& min, const VectorType& max);

  /**
   * @brief Copy constructor.
   *
   * @tparam OtherT - Real type of the box being copied.
   * @param[in] other - The box to be copied.
   */
  template <typename OtherT>
  AlignedBox(const AlignedBox<OtherT, N>& other);

  /**
   * @brief Reset the box to empty.
   */
  void Reset();

  /**
   * @brief Reset the box to a new min/max.
   *
   * If the any components of min are greater than max an
   * empty box will result.
   *
   * @param[in] min - The box minimum point.
   * @param[in] max - The box maximum point.
   */
  void Reset(const VectorType& min, const VectorType& max);

  /**
   * @brief Test if the box is empty.
   *
   * @return - True if the box is empty.
   */
  bool Empty() const;

  /**
   * @brief Get the center of the box.
   *
   * The center of an empty box is undefined.
   *
   * @return - Box center point.
   */
  VectorType Center() const;

  /**
   * @brief Get the size of the box.
   *
   * The size of an empty box is zero.
   *
   * @return - The size along each dimension.
   * @note overflow might happen. E.g. max is INT_MAX and min is INT_MIN.
   */
  VectorType Size() const;

  /**
   * @brief Get the box minimum corner point.
   *
   * The minimum corner point of an empty box is undefined.
   *
   * @return - Box minimum point.
   */
  const VectorType& Minimum() const;

  /**
   * @brief Get the box maximum corner point.
   *
   * The maximum corner point of an empty box is undefined.
   *
   * @return - Box maximum point.
   */
  const VectorType& Maximum() const;

  /**
   * @brief Get the corner points of the box.
   *
   * The corner points of an empty box are undefined.
   *
   * @return - An array containing the box corner points.
   */
  CornerArrayType Corners() const;
  /**
   * @brief Test if a box contains a point.
   *
   * This test is inclusive.
   *
   * @param[in] point - The point to be tested.
   *
   * @return - True if the point is contained by the box.
   */
  bool Contains(const VectorType& point, T epsilon = T()) const;

  /**
   * @brief Test if a box contains another box
   *
   * @param[in] box Aligned box
   *
   * @return True if the box is contained by the box.
   */
  bool Contains(const AlignedBox& box) const;

  /**
   * @brief Test if a box intersects another box.
   *
   * The test box is considered to be insecting if it is
   * contained by the box.
   *
   * @param[in] box - The box to be tested.
   *
   * @return - True if the boxes intersect.
   */
  bool Intersects(const AlignedBox& box) const;

  /**
   * @brief Compute the nearest point on the box to a point.
   *
   * The nearest point to an empty box is undefined.
   *
   * @param[in] point - Point from which to find the nearest point.
   *
   * @return - The nearest point on the box to the specified point.
   */
  VectorType NearestPoint(const VectorType& point) const;

  /**
   * @brief Compute the distance to the box.
   *
   * A point on or inside the box will have a distance of zero.
   *
   * The distance to an empty box is undefined.
   *
   * @param[in] point - Point from which to find the distance.
   *
   * @return - The distance between the point and the box.
   */
  ValueType Distance(const VectorType& point) const;

  /**
   * @brief Compute the squared distance to the box.
   *
   * A point on or inside the box will have a squared distance of zero.
   *
   * The squared distance to an empty box is undefined.
   *
   * @param[in] point - Point from which to find the squared distance.
   *
   * @return - The squared distance between the point and the box.
   */
  ValueType Distance2(const VectorType& point) const;

  /**
   * @brief Equality operator.
   *
   * @param[in] box - Other box.
   *
   * @return - True if the boxes are equal.
   */
  bool operator==(const AlignedBox& box) const;

  /**
   * @brief Inequality operator.
   *
   * @param[in] box - Other box.
   *
   * @return - True if the boxes are not equal.
   */
  bool operator!=(const AlignedBox& box) const;

 private:
  template <typename U, unsigned int X>
  friend class AlignedBox;

  VectorType minimum_; /*!< Box min point. */
  VectorType maximum_; /*!< Box max point. */
};

#ifndef NDEBUG
#define CHECK_INT_ADD_OPERATION_OVERFLOWS(a, b, Type)                      \
  (((a > 0) && (b > 0) && (a > (std::numeric_limits<Type>::max() - b))) || \
   ((a < 0) && (b < 0) && (a < (std::numeric_limits<Type>::min() - b))))
#endif

using AlignedBox3d =
    AlignedBox<double, 3>; /*!< Three dimension double-precision box type. */

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
  // Check for possible 64-bit integer overflow/underflow
  if (std::numeric_limits<OverflowType>::is_integer &&
      sizeof(OverflowType) * 8 == 64) {
    for (unsigned dim = 0; dim < dimensions; ++dim) {
      if (CHECK_INT_ADD_OPERATION_OVERFLOWS(maximum_[dim], minimum_[dim],
                                            OverflowType)) {
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
  // If either box is empty than check which ones are
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
