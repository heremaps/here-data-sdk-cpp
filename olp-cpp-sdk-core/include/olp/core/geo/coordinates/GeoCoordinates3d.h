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
 * @brief Geodetic coordinates with longitude, latitude, and altitude.
 */
class CORE_API GeoCoordinates3d {
 public:
  /**
   * @brief Creates a `GeoCoordinates3d` instance with invalid geodetic coordinates.
   *
   * @post The latitude, longitude, and altitude are undefined (NaN).
   */
  GeoCoordinates3d();

  /**
   * @brief Creates a `GeoCoordinates3d` instance from latitude,
   * longitude, and altitude.
   *
   * @param[in] latitude_radians The latitude in radians.
   * @param[in] longitude_radians The longitude in radians.
   * @param[in] altitude_meters The altitude in meters.
   */
  GeoCoordinates3d(double latitude_radians, double longitude_radians,
                   double altitude_meters);

  /**
   * @brief Creates a `GeoCoordinates3d` instance from latitude,
   * longitude, and altitude.
   * 
   * @param latitude_degrees The latitude in degrees.
   * @param longitude_degrees The longitude in degrees.
   * @param altitude_meters The altitude in meters.
   * @param degrees The degree tag.
   */
  GeoCoordinates3d(double latitude_degrees, double longitude_degrees,
                   double altitude_meters, DegreeType degrees);

  /**
   * @brief Creates a `GeoCoordinates3d` instance from 2D coordinates
   * with undefined altitude.
   *
   * @param[in] geo_coordinates The 2D geodetic coordinates.
   *
   * @post The altitude is undefined (NaN).
   */
  explicit GeoCoordinates3d(const GeoCoordinates& geo_coordinates);

  /**
   * @brief Creates a `GeoCoordinates3d` instance from 2D coordinates
   * and altitude.
   *
   * @param[in] geo_coordinates The 2D geodetic coordinates.
   * @param[in] altitude_meters The altitude in meters.
   */
  GeoCoordinates3d(const GeoCoordinates& geo_coordinates,
                   double altitude_meters);

  /**
   * @brief Creates a `GeoCoordinates3d` instance from latitude,
   * longitude, and altitude.
   *
   * @param[in] latitude The latitude in degrees.
   * @param[in] longitude The longitude in degrees.
   * @param[in] altitude The altitude in meters.
   *
   * @return The `GeoCoordinates3d` instance.
   */
  static GeoCoordinates3d FromDegrees(double latitude, double longitude,
                                      double altitude = 0.0);

  /**
   * @brief Creates a `GeoCoordinates3d` instance from latitude,
   * longitude, and altitude.
   *
   * @param[in] latitude The latitude in radians.
   * @param[in] longitude The longitude in radians.
   * @param[in] altitude The altitude in meters.
   *
   * @return The `GeoCoordinates3d` instance.
   */
  static GeoCoordinates3d FromRadians(double latitude, double longitude,
                                      double altitude = 0.0);

  /**
   * @brief Gets the latitude and longitude as 2D geodetic coordinates.
   *
   * @return The 2D geodetic coordinates.
   */
  const GeoCoordinates& GetGeoCoordinates() const;

  /**
   * @brief Sets the latitude and longitude from the 2D geodetic coordinates.
   *
   * @param[in] geo_coordinates The 2D geodetic coordinates.
   */
  void SetGeoCoordinates(const GeoCoordinates& geo_coordinates);

  /**
   * @brief Gets the latitude in radians.
   *
   * @return The latitude in radians.
   */
  double GetLatitude() const;

  /**
   * @brief Sets the latitude in radians.
   *
   * @param[in] latitude_radians The latitude in radians.
   */
  void SetLatitude(double latitude_radians);

  /**
   * @brief Gets the longitude in radians.
   *
   * @return The longitude in radians.
   */
  double GetLongitude() const;

  /**
   * @brief Sets the longitude in radians.
   *
   * @param[in] longitude_radians The longitude in radians.
   */
  void SetLongitude(double longitude_radians);

  /**
   * @brief Gets the latitude in degrees.
   *
   * @return The latitude in degrees.
   */
  double GetLatitudeDegrees() const;

  /**
   * @brief Sets the latitude in degrees.
   *
   * @param[in] latitude_degrees The latitude in degrees.
   */
  void setLatitudeDegrees(double latitude_degrees);

  /**
   * @brief Gets the longitude in degrees.
   *
   * @return The longitude in degrees.
   */
  double GetLongitudeDegrees() const;

  /**
   * @brief Sets the longitude in degrees.
   *
   * @param[in] longitude_degrees The longitude in degrees.
   */
  void SetLongitudeDegrees(double longitude_degrees);

  /**
   * @brief Gets the altitude in meters.
   *
   * @return The altitude in meters.
   */
  double GetAltitude() const;

  /**
   * @brief Sets the altitude in meters.
   *
   * @param[in] altitude_meters The altitude in meters.
   */
  void SetAltitude(double altitude_meters);

  /**
   * @brief Checks whether the coordinates and altitude are valid.
   *
   * @return True if the coordinates and altitude are valid;
   * false if the latitude, longitude, or altitude is undefined.
   */
  explicit operator bool() const;

  /**
   * @brief Checks whether the coordinates and altitude are valid.
   *
   * @return True if the coordinates and altitude are valid;
   * false if the latitude, longitude, or altitude is undefined.
   */
  bool IsValid() const;

 protected:
  /// The 2D geodetic coordinates.
  GeoCoordinates geo_coordinates_;
  /// The altitude in meters.
  double altitude_;
  /// The const that signalizes invalid altitudes.
  static const double kNaN_;
};

}  // namespace geo
}  // namespace olp
