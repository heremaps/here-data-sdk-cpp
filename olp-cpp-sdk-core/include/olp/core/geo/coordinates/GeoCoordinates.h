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

#include <olp/core/CoreApi.h>
#include <olp/core/geo/Types.h>
#include <olp/core/geo/coordinates/GeoPoint.h>

namespace olp {
namespace geo {

/**
 * @brief A geographic location that uses the WGS84 Coordinate System.
 *
 * Latitude values range from 0 at the equator to 90 degrees north and -90 degrees
 * south. Longitude values range from 0 at the prime meridian to 180 degrees
 * east and -180 degrees west.
 *
 * Internal representation of angles is radians.
 */
class CORE_API GeoCoordinates {
 public:
  /// Creates a `GeoCoordinates` instance with invalid coordinates.
  GeoCoordinates();

  /**
   * @brief Creates a `GeoCoordinates` instance from latitude and longitude.
   *
   * @note Use `Normalized()` to put a coordinate in a valid range.
   *
   * @param latitude_radians The WGS84 latitude in radians. Valid values are in
   * the [-pi/2, pi/2] range.
   * @param longitude_radians The WGS84 longitude in radians. Valid values are in
   * the [-pi, pi] range.
   */
  GeoCoordinates(double latitude_radians, double longitude_radians);

  /**
   * @brief Creates a `GeoCoordinates` instance from latitude and longitude.
   *
   * @note Use `Normalized()` to put a coordinate in a valid range.
   *
   * @param latitude_degrees The WGS84 latitude in degrees. Valid values are in
   * the [-90, 90] range.
   * @param longitude_degrees The WGS84 longitude in degrees. Valid values are in
   * the [-180, 180] range.
   * @param degrees The dispatch tag for coordinates in degrees.
   */
  GeoCoordinates(double latitude_degrees, double longitude_degrees,
                 DegreeType degrees);

  /**
   * @brief Creates a `GeoCoordinates` instance from latitude and longitude.
   *
   * @note Use `Normalized()` to put a value in a valid range.
   *
   * @param latitude_degrees The WGS84 latitude in degrees. Valid values are in
   * the [-90, 90] range.
   * @param longitude_degrees The WGS84 longitude in degrees. Valid values are in
   * the [-180, 180] range.
   *
   * @return The `GeoCoordinates` instance based on the specified
   * latitude and longitude.
   */
  static GeoCoordinates FromDegrees(double latitude_degrees,
                                    double longitude_degrees);

  /**
   * @brief Creates a `GeoCoordinates` instance from a geo point.
   *
   * @param geo_point The geo point.
   *
   * @return The `GeoCoordinates` instance based on the specified geo point.
   */
  static GeoCoordinates FromGeoPoint(const GeoPoint& geo_point);

  /**
   * @brief Converts the current coordinates to a geo point.
   *
   * @return The current coordinates as a geo point.
   */
  GeoPoint ToGeoPoint() const;

  /**
   * @brief Gets the WGS84 latitude in radians.
   *
   * @return The WGS84 latitude in radians.
   */
  double GetLatitude() const;

  /**
   * @brief Sets the latitude in radians.
   *
   * @param latitude_radians The WGS84 latitude in radians.
   * Valid values are in the [-pi/2, pi/2] range.
   */
  void SetLatitude(double latitude_radians);

  /**
   * @brief Gets the WGS84 longitude in radians.
   *
   * @return The WGS84 longitude in radians.
   */
  double GetLongitude() const;

  /**
   * @brief Sets the longitude in radians.
   *
   * @param longitude_radians The WGS84 longitude in radians.
   * Valid values are in the [-pi, pi] range.
   */
  void SetLongitude(double longitude_radians);

  /**
   * @brief Gets the WGS84 latitude in degrees.
   *
   * @return The WGS84 latitude in degrees.
   */
  double GetLatitudeDegrees() const;

  /**
   * @brief Sets the latitude in degrees.
   *
   * @param latitude_degrees The WGS84 latitude in degrees.
   * Valid values are in the [-90, 90] range.
   */
  void SetLatitudeDegrees(double latitude_degrees);

  /**
   * @brief Gets the WGS84 longitude in degrees.
   *
   * @return The WGS84 longitude in degrees.
   */
  double GetLongitudeDegrees() const;

  /**
   * @brief Sets the longitude in degrees.
   *
   * @param longitude_degrees The WGS84 longitude in degrees.
   * Valid values are in the [-180, 180] range.
   */
  void SetLongitudeDegrees(double longitude_degrees);

  /**
   * @brief Normalizes the latitude and longitude to 
   * the [-pi/2, pi/2] and [-pi, pi] ranges correspondingly.
   */
  GeoCoordinates Normalized() const;

  /**
   * @brief Overloads the bool operator.
   *
   * @see `IsValid`
   *
   * @return True if the coordinates are valid; false otherwise.
   */
  explicit operator bool() const;

  /**
   * @brief Checks whether the radian values of latitude and longitude
   * are valid double numbers.
   *
   * The check happens with the help of `math::isnan`.
   *
   * @return True if the result of the check is positive; false otherwise.
   */
  bool IsValid() const;

 private:
  /// The latitude in radians.
  double latitude_;
  /// The longitude in radians.
  double longitude_;
  /// The const that signalizes an invalid value of latitude or longitude.
  static const double kNaN_;
};

/**
 * @brief Checks whether two coordinates are equal.
 *
 * @return True if the coordinates are equal; false otherwise.
 */
CORE_API bool operator==(const GeoCoordinates& lhs, const GeoCoordinates& rhs);

}  // namespace geo
}  // namespace olp
