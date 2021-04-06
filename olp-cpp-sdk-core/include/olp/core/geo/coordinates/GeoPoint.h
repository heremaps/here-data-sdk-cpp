/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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
 * @brief A geographic location that uses the WGS84 Coordinate System
 * encoded in a 32-bit unsigned integer.
 *
 * Latitude values range from 0 at the equator to 90 degrees north and
 * -90 degrees south. Longitude values range from 0 at the prime
 * meridian to 180 degrees east and -180 degrees west.
 *
 * The X-Y coordinates system is used to get the geographic location:
 * - x – a longitude represented as a 32-bit unsigned integer.
 * - y – a latitude represented as a 32-bit unsigned integer.
 *
 * The internal representation of angles is radians:
 * - x rad = -180 ... +180 (-pi ... pi)
 * - y rad = -90 ... +90 (-pi/2 ... pi/2)
 *
 * To get `GeoPoint` from geographic coordinates, you can use the `ToGeoPoint`
 * method of the `GeoCoordinates` class.
 */
class CORE_API GeoPoint {
 public:
  GeoPoint();

  /**
   * @brief Creates the `GeoPoint` instance that uses the location longitude
   * (`x`) and latitude (`y`) values represented as 32-bit unsigned integers.
   *
   * @param[in] xx The X-coordinate value of the location longitude.
   * @param[in] yy The Y-coordinate value of the location latitude.
   */
  GeoPoint(uint32_t xx, uint32_t yy);

  /**
   * @brief An absolute world X-coordinate value.
   *
   * The value range for each component is a 32-bit unsigned integer.
   *
   * The `x` value can be calculated using the following formula:
   * `x = (x rad + pi) * max(uint32_t) / 2 * pi`.
   */
  std::uint32_t x;

  /**
   * @brief An absolute world Y-coordinate value.
   *
   * The value range for each component is a 32-bit unsigned integer.
   *
   * The `y` value can be calculated using the following formula:
   * `y = (y rad + pi/2) * max(uint32_t) / 2 * pi`.
   */
  std::uint32_t y;

  /**
   * @brief Checks whether the values of the `GeoPoint` and the `other`
   * parameters are equal.
   *
   * @param other The `GeoPoint` instance.
   *
   * @return True if the values are equal; false otherwise.
   */
  bool operator==(const GeoPoint& other) const;

  /**
   * @brief Checks whether the values of the `GeoPoint` and the `other`
   * parameters are not equal.
   *
   * @param other The `GeoPoint` instance.
   *
   * @return True if the values are not equal; false otherwise.
   */
  inline bool operator!=(const GeoPoint& other) const {
    return !operator==(other);
  }

  /**
   * @brief Adds the values of the `GeoPoint` and `other` parameters.
   *
   * @param other The `GeoPoint` instance.
   *
   * @return The reference to the `GeoPoint` instance.
   */
  GeoPoint& operator+=(const GeoPoint& other);
};

inline GeoPoint::GeoPoint() : x(0), y(0) {}

inline GeoPoint::GeoPoint(uint32_t xx, uint32_t yy) : x(xx), y(yy) {}

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
