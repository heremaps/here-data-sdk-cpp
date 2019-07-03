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

#include <olp/core/CoreApi.h>
#include <olp/core/geo/Types.h>
#include <olp/core/geo/coordinates/GeoPoint.h>

namespace olp {
namespace geo {
/**
 * @brief A geographic location using WGS84 coordinates.
 *
 * Latitude values range from 0 at equator to 90 degrees north and -90 degrees
 * south. Longitude values range from 0 at prime meridian to 180 degrees
 * eastwards and -180 degrees westwards.
 *
 * Internal representation of angles is radians.
 */
class CORE_API GeoCoordinates {
 public:
  /**
   * @brief Construct default geo coordinates
   */
  GeoCoordinates();

  /**
   * @brief Construct geo coordinates form latitude and longitude
   * @param latitude_radians WGS84 latitude in radians. Valid values are in
   * [-Pi/2, Pi/2] range.
   * @param longitude_radians WGS84 longitude in radians. Valid values are in
   * [-Pi, Pi) range. Use normalized() to put geo coordinate in a valid range.
   */
  GeoCoordinates(double latitude_radians, double longitude_radians);

  /**
   * @brief Construct geo coordinates form latitude and longitude in degrees
   * @param latitude_degrees WGS84 latitude in degrees. Valid values are in
   * [-90, 90] range.
   * @param longitude_degrees WGS84 longitude in degrees. Valid values are in
   * [-180, 180) range. Use normalized() to put geo coordinate in a valid range.
   */
  GeoCoordinates(double latitude_degrees, double longitude_radians,
                 DegreeType degrees);

  /**
   * @brief Create GeoCoordinates from latitude and longitude in degrees.
   * @param latitude_degrees WGS84 latitude in degrees. Valid value is in [-90,
   * 90] range.
   * @param longitude_degrees WGS84 longitude in degrees. Valid value is in
   * [-180, 180) range.
   * @return GeoCoordinate based on specified input.
   * Use normalized() to put values in a valid range.
   */
  static GeoCoordinates FromDegrees(double latitude_degrees,
                                    double longitude_degrees);

  /**
   * @brief Create GeoCoordinates from GeoPoint.
   * @param geo_point Geo point to convert to.
   * @return GeoCoordinate based on specified geoPoint.
   */
  static GeoCoordinates FromGeoPoint(const GeoPoint& geo_point);

  /**
   * @brief Return geo coordinate as GeoPoint
   * @return current coordinate as a GeoPoint.
   */
  GeoPoint ToGeoPoint() const;

  double GetLatitude() const;

  void SetLatitude(double latitude_radians);

  double GetLongitude() const;

  void SetLongitude(double latitude_radians);

  double GetLatitudeDegrees() const;

  void SetLatitudeDegrees(double latitude_degrees);

  double GetLongitudeDegrees() const;

  void SetLongitudeDegrees(double longitude_degrees);

  /**
   * @brief Normalize the latitude and longitude to [-Pi/2, Pi/2], [-Pi, Pi)
   * range. To that end, longitude wraps around at +/- Pi and latitude is
   * clamped at +/- Pi/2
   */
  GeoCoordinates Normalized() const;

  explicit operator bool() const;

  bool IsValid() const;

 private:
  double latitude_;   ///< Latitude in radians.
  double longitude_;  ///< Longitude in radians.
  static const double kNaN_;
};

bool operator==(const GeoCoordinates& lhs, const GeoCoordinates& rhs);

}  // namespace geo
}  // namespace olp
