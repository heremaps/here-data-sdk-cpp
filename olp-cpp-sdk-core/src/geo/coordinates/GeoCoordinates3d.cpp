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

#include "olp/core/geo/coordinates/GeoCoordinates3d.h"

#include "olp/core/math/Math.h"

namespace olp {
namespace geo {

const double GeoCoordinates3d::kNaN_ = std::numeric_limits<double>::quiet_NaN();

GeoCoordinates3d::GeoCoordinates3d() : geo_coordinates_(), altitude_(kNaN_) {}

GeoCoordinates3d::GeoCoordinates3d(double latitude_radians,
                                   double longitude_radians,
                                   double altitude_meters)
    : geo_coordinates_(latitude_radians, longitude_radians),
      altitude_(altitude_meters) {}

GeoCoordinates3d::GeoCoordinates3d(double latitude_degrees,
                                   double longitude_degrees,
                                   double altitude_meters, DegreeType degrees)
    : geo_coordinates_(latitude_degrees, longitude_degrees, degrees),
      altitude_(altitude_meters) {}

GeoCoordinates3d::GeoCoordinates3d(const GeoCoordinates& geo_coordinates)
    : geo_coordinates_(geo_coordinates), altitude_(kNaN_) {}

GeoCoordinates3d::GeoCoordinates3d(const GeoCoordinates& geo_coordinates,
                                   double altitude_meters)
    : geo_coordinates_(geo_coordinates), altitude_(altitude_meters) {}

GeoCoordinates3d GeoCoordinates3d::FromDegrees(double latitude,
                                               double longitude,
                                               double altitude) {
  return GeoCoordinates3d(math::Radians(latitude), math::Radians(longitude),
                          altitude);
}

GeoCoordinates3d GeoCoordinates3d::FromRadians(double latitude,
                                               double longitude,
                                               double altitude) {
  return GeoCoordinates3d(latitude, longitude, altitude);
}

const GeoCoordinates& GeoCoordinates3d::GetGeoCoordinates() const {
  return geo_coordinates_;
}

void GeoCoordinates3d::SetGeoCoordinates(
    const GeoCoordinates& geo_coordinates) {
  geo_coordinates_ = geo_coordinates;
}

double GeoCoordinates3d::GetLatitude() const {
  return geo_coordinates_.GetLatitude();
}

void GeoCoordinates3d::SetLatitude(double latitude_radians) {
  geo_coordinates_.SetLatitude(latitude_radians);
}

double GeoCoordinates3d::GetLongitude() const {
  return geo_coordinates_.GetLongitude();
}

void GeoCoordinates3d::SetLongitude(double longitude_radians) {
  geo_coordinates_.SetLongitude(longitude_radians);
}

double GeoCoordinates3d::GetLatitudeDegrees() const {
  return geo_coordinates_.GetLatitudeDegrees();
}

void GeoCoordinates3d::setLatitudeDegrees(double latitude_degrees) {
  geo_coordinates_.SetLatitudeDegrees(latitude_degrees);
}

double GeoCoordinates3d::GetLongitudeDegrees() const {
  return geo_coordinates_.GetLongitudeDegrees();
}

void GeoCoordinates3d::SetLongitudeDegrees(double longitude_degrees) {
  geo_coordinates_.SetLongitudeDegrees(longitude_degrees);
}

double GeoCoordinates3d::GetAltitude() const { return altitude_; }

void GeoCoordinates3d::SetAltitude(double altitude_meters) {
  altitude_ = altitude_meters;
}

GeoCoordinates3d::operator bool() const { return IsValid(); }

bool operator==(const GeoCoordinates3d& lhs, const GeoCoordinates3d& rhs) {
  return math::EpsilonEqual(lhs.GetAltitude(), rhs.GetAltitude()) &&
         lhs.GetGeoCoordinates() == rhs.GetGeoCoordinates();
}

bool operator!=(const GeoCoordinates3d& lhs, const GeoCoordinates3d& rhs) {
  return !(lhs == rhs);
}

bool GeoCoordinates3d::IsValid() const {
  return !math::isnan(altitude_) && geo_coordinates_.IsValid();
}

}  // namespace geo
}  // namespace olp
