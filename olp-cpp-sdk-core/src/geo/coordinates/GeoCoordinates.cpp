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

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/math/Math.h>

namespace olp {

using namespace math;

namespace geo {
const double GeoCoordinates::kNaN_ = std::nan("");

GeoCoordinates GeoCoordinates::FromGeoPoint(const GeoPoint& geo_point) {
  constexpr std::uint32_t maxUInt32 = std::numeric_limits<std::uint32_t>::max();
  const double uInt32ToRadFactor = math::two_pi / maxUInt32;
  return GeoCoordinates{geo_point.y * uInt32ToRadFactor - math::half_pi,
                        geo_point.x * uInt32ToRadFactor - math::pi};
}

GeoPoint GeoCoordinates::ToGeoPoint() const {
  const GeoCoordinates norm = Normalized();
  constexpr std::uint32_t maxUInt32 = std::numeric_limits<std::uint32_t>::max();
  const double radToUInt32Factor = maxUInt32 * math::one_over_two_pi;

  const auto x = static_cast<std::uint32_t>(
      std::round((norm.longitude_ + pi) * radToUInt32Factor));
  const auto y = static_cast<std::uint32_t>(
      std::round((norm.latitude_ + half_pi) * radToUInt32Factor));

  return {x, y};
}

GeoCoordinates::GeoCoordinates() : latitude_(kNaN_), longitude_(kNaN_) {}

GeoCoordinates::GeoCoordinates(double latitude_radians,
                               double longitude_radians)
    : latitude_(latitude_radians), longitude_(longitude_radians) {}

GeoCoordinates::GeoCoordinates(double latitude_degrees,
                               double longitude_degrees, DegreeType)
    : latitude_(math::Radians(latitude_degrees)),
      longitude_(math::Radians(longitude_degrees)) {}

GeoCoordinates GeoCoordinates::FromDegrees(double latitude_degrees,
                                           double longitude_degrees) {
  return GeoCoordinates(math::Radians(latitude_degrees),
                        math::Radians(longitude_degrees));
}

double GeoCoordinates::GetLatitude() const { return latitude_; }

void GeoCoordinates::SetLatitude(double latitude_radians) {
  latitude_ = latitude_radians;
}

double GeoCoordinates::GetLongitude() const { return longitude_; }

void GeoCoordinates::SetLongitude(double longitude_radians) {
  longitude_ = longitude_radians;
}

GeoCoordinates::operator bool() const { return IsValid(); }

bool operator==(const GeoCoordinates& lhs, const GeoCoordinates& rhs) {
  return math::EpsilonEqual(lhs.GetLatitude(), rhs.GetLatitude()) &&
         math::EpsilonEqual(lhs.GetLongitude(), rhs.GetLongitude());
}

bool operator!=(const GeoCoordinates& lhs, const GeoCoordinates& rhs) {
  return !(lhs == rhs);
}

double GeoCoordinates::GetLatitudeDegrees() const {
  return math::Degrees(latitude_);
}

void GeoCoordinates::SetLatitudeDegrees(double latitudeDegrees) {
  latitude_ = math::Radians(latitudeDegrees);
}

double GeoCoordinates::GetLongitudeDegrees() const {
  return math::Degrees(longitude_);
}

void GeoCoordinates::SetLongitudeDegrees(double longitude_degrees) {
  longitude_ = math::Radians(longitude_degrees);
}

GeoCoordinates GeoCoordinates::Normalized() const {
  if (!IsValid()) {
    return *this;
  }

  GeoCoordinates norm{*this};
  norm.latitude_ = math::Clamp(norm.latitude_, -math::half_pi, +math::half_pi);
  norm.longitude_ = math::Wrap(norm.longitude_, -math::pi, +math::pi);

  return norm;
}

bool GeoCoordinates::IsValid() const {
  return !math::isnan(latitude_) && !math::isnan(longitude_);
}

}  // namespace geo
}  // namespace olp
