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

#include "olp/core/geo/coordinates/GeoRectangle.h"

#include <algorithm>

#include "olp/core/math/Math.h"

namespace olp {
namespace geo {

/** Constructs an empty rectangle. */
GeoRectangle::GeoRectangle()
    : south_west_(math::half_pi, math::pi),
      north_east_(-math::half_pi, -math::pi) {}

GeoRectangle::GeoRectangle(const GeoCoordinates& south_west,
                           const GeoCoordinates& north_east)
    : south_west_(south_west), north_east_(north_east) {}

bool GeoRectangle::IsEmpty() const { return LatitudeSpan() < 0; }

GeoCoordinates GeoRectangle::SouthEast() const {
  return GeoCoordinates(south_west_.GetLatitude(), north_east_.GetLongitude());
}

GeoCoordinates GeoRectangle::SouthWest() const { return south_west_; }

GeoCoordinates GeoRectangle::NorthEast() const { return north_east_; }

GeoCoordinates GeoRectangle::NorthWest() const {
  return GeoCoordinates(north_east_.GetLatitude(), south_west_.GetLongitude());
}

double GeoRectangle::LatitudeSpan() const {
  return north_east_.GetLatitude() - south_west_.GetLatitude();
}

double GeoRectangle::LongitudeSpan() const {
  double width = north_east_.GetLongitude() - south_west_.GetLongitude();
  if (width < 0) {
    width += math::two_pi;
  }

  return width;
}

GeoCoordinates GeoRectangle::Center() const {
  const double latitude =
      (south_west_.GetLatitude() + north_east_.GetLatitude()) * 0.5;

  const double west = south_west_.GetLongitude();
  const double east = north_east_.GetLongitude();

  if (west < east) {
    return GeoCoordinates(latitude, (west + east) * 0.5);
  }

  double longitude = (math::two_pi + east + west) * 0.5;
  if (longitude > math::two_pi) {
    longitude -= math::two_pi;
  }

  return GeoCoordinates(latitude, longitude);
}

bool GeoRectangle::Contains(const GeoCoordinates& point) const {
  if (point.GetLatitude() < south_west_.GetLatitude() ||
      point.GetLatitude() > north_east_.GetLatitude())
    return false;

  const double west = south_west_.GetLongitude();
  const double east = north_east_.GetLongitude();

  if (east > west) {
    return point.GetLongitude() >= west && point.GetLongitude() <= east;
  }

  // this code handles rectangle crossing 180 meridian
  return point.GetLongitude() >= east || point.GetLongitude() <= west;
}

bool GeoRectangle::Overlaps(const GeoRectangle& rectangle) const {
  if (south_west_.GetLatitude() >= rectangle.north_east_.GetLatitude() ||
      rectangle.south_west_.GetLatitude() >= north_east_.GetLatitude()) {
    return false;
  }

  double west = south_west_.GetLongitude();
  double east = north_east_.GetLongitude();

  if (west >= east) {
    east = west + LatitudeSpan();
  }

  double rectangle_west = rectangle.south_west_.GetLongitude();
  double rectangle_east = rectangle.north_east_.GetLongitude();

  if (rectangle_west >= rectangle_east) {
    rectangle_east = rectangle_west + rectangle.LatitudeSpan();
  }

  return !(west >= rectangle_east || rectangle_west >= east);
}

GeoRectangle GeoRectangle::BooleanUnion(const GeoRectangle& other) const {
  if (IsEmpty()) {
    return other;
  }

  if (other.IsEmpty()) {
    return *this;
  }

  const GeoCoordinates south_west(
      std::min(south_west_.GetLatitude(), other.south_west_.GetLatitude()),
      std::min(south_west_.GetLongitude(), other.south_west_.GetLongitude()));

  // Special handling to cover longitude wrapping
  double longitude1 = north_east_.GetLongitude();
  if (longitude1 < south_west_.GetLongitude()) {
    longitude1 += math::two_pi;
  }

  double longitude2 = other.north_east_.GetLongitude();
  if (longitude2 < other.south_west_.GetLongitude()) {
    longitude2 += math::two_pi;
  }

  double max_longitude = std::max(longitude1, longitude2);
  if (max_longitude > math::pi) {
    const double upper_limit =
        nextafter(south_west.GetLongitude(), south_west.GetLongitude() - 1.0);
    max_longitude = std::min(max_longitude - math::two_pi, upper_limit);
  }

  const GeoCoordinates north_east(
      std::max(north_east_.GetLatitude(), other.north_east_.GetLatitude()),
      max_longitude);

  return GeoRectangle(south_west, north_east);
}

GeoRectangle& GeoRectangle::GrowToContain(const GeoCoordinates& point) {
  double point_latitude = point.GetLatitude();
  double point_longitude = point.GetLongitude();

  if (point_latitude < south_west_.GetLatitude())
    south_west_.SetLatitude(point_latitude);
  if (point_latitude > north_east_.GetLatitude())
    north_east_.SetLatitude(point_latitude);
  if (point_longitude < south_west_.GetLongitude())
    south_west_.SetLongitude(point_longitude);
  if (point_longitude > north_east_.GetLongitude())
    north_east_.SetLongitude(point_longitude);

  return *this;
}

}  // namespace geo
}  // namespace olp
