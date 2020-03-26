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
 * Geodetic coordinates with longitude, latitude and altitude.
 */
class CORE_API GeoCoordinates3d {
 public:
  /**
   * @brief Construct invalid geodetic coordinates.
   *
   * @post Latitude, longitude and altitude are undefined (NaN).
   */
  GeoCoordinates3d();

  /**
   * @brief Construct geodetic coordinates from latitude, longitude and
   * altitude.
   *
   * @param[in] latitude_radians Latitude in radians.
   * @param[in] longitude_radians Longitude in radians.
   * @param[in] altitude_meters Altitude in meters.
   */
  GeoCoordinates3d(double latitude_radians, double longitude_radians,
                   double altitude_meters);

  /**
   * @brief Construct geodetic coordinates from latitude, longitude and
   * altitude.
   *
   * @param[in] latitudeDegrees Latitude in degrees.
   * @param[in] longitudeDegrees Longitude in degrees.
   * @param[in] altitude_meters Altitude in meters.
   * @param[in] degrees Degree tag.
   */
  GeoCoordinates3d(double latitude_degrees, double longitude_degrees,
                   double altitude_meters, DegreeType degrees);

  /**
   * @brief Construct geodetic coordinates from 2D coordinates with undefined
   * altitude.
   *
   * @param[in] geo_coordinates 2D geodetic coordinates.
   * @post Altitude is undefined (NaN).
   */
  explicit GeoCoordinates3d(const GeoCoordinates& geo_coordinates);

  /**
   * @brief Construct geodetic coordinates from 2D coordinates and an altitude.
   *
   * @param[in] geo_coordinates 2D geodetic coordinates.
   * @param[in] altitude_meters Altitude in meters.
   */
  GeoCoordinates3d(const GeoCoordinates& geo_coordinates,
                   double altitude_meters);

  /**
   * @brief Create geodetic coordinates from latitude and longitude in degrees.
   *
   * @param[in] latitude Latitude in degrees.
   * @param[in] longitude Longitude in degrees.
   * @param[in] altitude Altitude in meters.
   * @return Geodetic coordinates.
   */
  static GeoCoordinates3d FromDegrees(double latitude, double longitude,
                                      double altitude = 0.0);

  /**
   * @brief Create geodetic coordinates from latitude and longitude in radians.
   *
   * @param[in] latitude Latitude in radians.
   * @param[in] longitude Longitude in radians.
   * @param[in] altitude Altitude in meters.
   * @return Geodetic coordinates.
   */
  static GeoCoordinates3d FromRadians(double latitude, double longitude,
                                      double altitude = 0.0);

  /**
   * @brief Get latitude and longitude as 2D geodetic coordinates.
   *
   * @return 2D geodetic coordinates.
   */
  const GeoCoordinates& GetGeoCoordinates() const;

  /**
   * @brief Set latitude and longitude from 2D geodetic coordinates.
   *
   * @param[in] geo_coordinates 2D geodetic coordinates.
   */
  void SetGeoCoordinates(const GeoCoordinates& geo_coordinates);

  /**
   * @brief Get latitude.
   *
   * @return Latitude in radians.
   */
  double GetLatitude() const;

  /**
   * @brief Set latitude.
   *
   * @param[in] latitude_radians Latitude in radians.
   */
  void SetLatitude(double latitude_radians);

  /**
   * @brief Get longitude.
   *
   * @return Longitude in radians.
   */
  double GetLongitude() const;

  /**
   * @brief Set longitude.
   *
   * @param[in] longitude_radians Longitude in radians.
   */
  void SetLongitude(double longitude_radians);

  /**
   * @brief Get latitude in degrees.
   *
   * @return Latitude in degrees.
   */
  double GetLatitudeDegrees() const;

  /**
   * @brief Set latitude in degrees.
   *
   * @param[in] latitude_degrees Latitude in degrees.
   */
  void setLatitudeDegrees(double latitude_degrees);

  /**
   * @brief Get longitude in degrees.
   * @return Longitude in degrees.
   */
  double GetLongitudeDegrees() const;

  /**
   * @brief Set longitude in degrees.
   *
   * @param[in] longitude_degrees Longitude in degrees.
   */
  void SetLongitudeDegrees(double longitude_degrees);

  /**
   * @brief Get altitude.
   *
   * @return Altitude in meters.
   */
  double GetAltitude() const;

  /**
   * @brief Set altitude.
   *
   * @param[in] altitude_meters Altitude in meters.
   */
  void SetAltitude(double altitude_meters);

  /**
   * @brief Check whether coordinates and altitude are valid.
   *
   * @return True if all valid, false if latitude, longitude or altitude is
   * undefined.
   */
  explicit operator bool() const;

  /**
   * @brief Check whether coordinates and altitude are valid.
   *
   * @return True if all valid, false if latitude, longitude or altitude is
   * undefined.
   */
  bool IsValid() const;

 protected:
  /// 2D geodetic coordinates.
  GeoCoordinates geo_coordinates_;
  /// Altitude in meters.
  double altitude_;
  /// Const being used to signalize an invalid altitude.
  static const double kNaN_;
};

}  // namespace geo
}  // namespace olp
