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

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace olp {
namespace math {
using std::isnan;
using std::max;
using std::min;
using std::pow;

constexpr double one_over_two_pi = 0.159154943091895335768883763372514362;
constexpr double half_pi = 1.57079632679489661923132169163975144;
constexpr double pi = 3.14159265358979323846264338327950288;
constexpr double two_pi = 6.28318530717958647692528676655900576;
constexpr double epsilon = std::numeric_limits<double>::epsilon();

CORE_API inline double Degrees(double radians) {
  return radians * 57.295779513082320876798154814105;
}

CORE_API inline double Radians(double degrees) {
  return degrees * 0.01745329251994329576923690768489;
}

CORE_API inline bool EpsilonEqual(double const& x, double const& y) {
  return std::abs(x - y) < epsilon;
}

CORE_API inline double Clamp(double const& x, double const& minVal,
                    double const& maxVal) {
  return min(max(x, minVal), maxVal);
}

//
// Provide integeral type modulus computation.

CORE_API inline double Wrap(double value, double lower, double upper) {
  // Return lower bound if the range is singular
  if (EpsilonEqual(lower, upper)) {
    return lower;
  }

  // Wrap around range or return exact unmodified value
  if (value < lower) {
    return upper - std::fmod((lower - value), (upper - lower));
  }

  if (upper <= value) {
    return lower + std::fmod((value - lower), (upper - lower));
  }
  return value;
}

}  // namespace math
}  // namespace olp
