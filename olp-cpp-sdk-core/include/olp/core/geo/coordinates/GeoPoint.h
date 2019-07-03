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

#include <cstdint>

#include <olp/core/CoreApi.h>

namespace olp {
namespace geo {
/**
 * @brief A geographic location using WGS84 coordinates encoded in 32bit
 * unsigned integer.
 */
class CORE_API GeoPoint {
 public:
  GeoPoint();

  /**
   * Construct the point
   * @param[in] xx X-coordinate
   * @param[in] yy Y-coordinate
   */
  GeoPoint(std::uint32_t xx, std::uint32_t yy);

  /**
   * 2D absolute world coordinate.
   * Value range for each component is 32 bit unsigned integer
   */
  std::uint32_t x;
  std::uint32_t y;

  bool operator==(const GeoPoint& other) const;
  inline bool operator!=(const GeoPoint& other) const {
    return !operator==(other);
  }

  GeoPoint& operator+=(const GeoPoint& other);
};

inline GeoPoint::GeoPoint() : x(0), y(0) {}

inline GeoPoint::GeoPoint(std::uint32_t xx, std::uint32_t yy) : x(xx), y(yy) {}

inline bool GeoPoint::operator==(const GeoPoint& other) const {
  return x == other.x && y == other.y;
}

inline GeoPoint& GeoPoint::operator+=(const GeoPoint& other) {
  x += other.x;
  y += other.y;
  return *this;
}

}  // namespace geo
}  // namespace olp
