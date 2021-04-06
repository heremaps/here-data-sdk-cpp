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

#include <olp/core/CoreApi.h>
#include <olp/core/geo/coordinates/GeoCoordinates.h>

namespace olp {
namespace geo {

/**
 * @brief A rectangular area in the WGS84 Coordinate System.
 *
 * @see `GeoBox` to add altitude.
 */
class CORE_API GeoRectangle {
 public:
  /// Creates an empty rectangle.
  GeoRectangle();

  /**
   * @brief Creates a `GeoRectangle` instance.
   * 
   * @param south_west The southwest corner of the rectangle.
   * @param north_east The northeast corner of the rectangle.
   */
  GeoRectangle(const GeoCoordinates& south_west,
               const GeoCoordinates& north_east);

  /**
   * @brief Computes the southeast corner of the rectangle.
   * 
   * @return The southeast corner of the rectangle.
   */
  GeoCoordinates SouthEast() const;

  /**
   * @brief Computes the southwest corner of the rectangle.
   * 
   * @return The southwest corner of the rectangle.
   */
  GeoCoordinates SouthWest() const;

  /**
   * @brief Computes the northeast corner of the rectangle.
   * 
   * @return The northeast corner of the rectangle.
   */
  GeoCoordinates NorthEast() const;

  /**
   * @brief Computes the northwest corner of the rectangle.
   * 
   * @return The northwest corner of the rectangle.
   */
  GeoCoordinates NorthWest() const;

  /**
   * @brief Computes the center of the rectangle.
   * 
   * @return The centere of the rectangle.
   */
  GeoCoordinates Center() const;

  /**
   * @brief Checks whether the rectangle is empty.
   * 
   * @return True if the rectangle is empty; false otherwise.
   */
  bool IsEmpty() const;

  /**
   * @brief Gets the longitude span of the rectangle.
   * 
   * @return The longitude span.
   */
  double LongitudeSpan() const;

  /**
   * @brief Gets the latitude span of the rectangle.
   * 
   * @return The latitude span. 
   */
  double LatitudeSpan() const;

  /**
   * @brief Checks whether the coordinates contain a point.
   * 
   * @param point The point to check.
   *
   * @return True if the coordinates contain a point; false otherwise.
   */
  bool Contains(const GeoCoordinates& point) const;

  /**
   * @brief Checks whether two rectangles overlap.
   * 
   * @param rectangle The other rectangle.
   *
   * @return True if the rectangles overlap; false otherwise.
   */
  bool Overlaps(const GeoRectangle& rectangle) const;

  /**
   * @brief Checks whether two rectangles are equal.
   * 
   * @param other The other rectangle.
   * 
   * @return True if the rectangles are equal; false otherwise.
   */
  bool operator==(const GeoRectangle& other) const;

  /**
   * @brief Checks whether two rectangles are not equal.
   * 
   * @param other The other rectangle.
   * 
   * @return True if the rectangles are not equal; false otherwise.
   */
  bool operator!=(const GeoRectangle& other) const;

  /**
   * @brief Unites two regtangles that have shared areas.
   * 
   * @param other The other rectangle.
   *
   * @return A new rectangle.
   */
  GeoRectangle BooleanUnion(const GeoRectangle& other) const;

  /**
   * @brief Expands the current rectangle so that it contains a point.
   * 
   * @param point The point to use for expanding.
   *
   * @return The new rectangle that contains the point.
   */
  GeoRectangle& GrowToContain(const GeoCoordinates& point);

 protected:
  /// The southwest corner of the rectangle.
  GeoCoordinates south_west_;
  /// The northeast corner of the rectangle.
  GeoCoordinates north_east_;
};

}  // namespace geo
}  // namespace olp
