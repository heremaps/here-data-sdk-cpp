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

#include <olp/core/geo/coordinates/GeoRectangle.h>

#include <algorithm>

#include <olp/core/math/Math.h>

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
  if (width < 0) width += math::two_pi;

  return width;
}

GeoCoordinates GeoRectangle::Center() const {
  const double latitude =
      (south_west_.GetLatitude() + north_east_.GetLatitude()) * 0.5;

  const double west = south_west_.GetLongitude();
  const double east = north_east_.GetLongitude();

  if (west < east) return GeoCoordinates(latitude, (west + east) * 0.5);

  double longitude = (math::two_pi + east + west) * 0.5;
  if (longitude > math::two_pi) longitude -= math::two_pi;

  return GeoCoordinates(latitude, longitude);
}

bool GeoRectangle::Contains(const GeoCoordinates& point) const {
  if (point.GetLatitude() < south_west_.GetLatitude() ||
      point.GetLatitude() > north_east_.GetLatitude())
    return false;

  const double west = south_west_.GetLongitude();
  const double east = north_east_.GetLongitude();

  if (east > west)
    return point.GetLongitude() >= west && point.GetLongitude() <= east;

  // this code handles rectangle crossing 180 meridian
  return point.GetLongitude() >= east || point.GetLongitude() <= west;
}

bool GeoRectangle::Overlaps(const GeoRectangle& rect) const {
  if (south_west_.GetLatitude() >= rect.north_east_.GetLatitude() ||
      rect.south_west_.GetLatitude() >= north_east_.GetLatitude())
    return false;

  double west = south_west_.GetLongitude();
  double east = north_east_.GetLongitude();

  if (west >= east) east = west + LatitudeSpan();

  double rectWest = rect.south_west_.GetLongitude();
  double rectEast = rect.north_east_.GetLongitude();

  if (rectWest >= rectEast) rectEast = rectWest + rect.LatitudeSpan();

  return !(west >= rectEast || rectWest >= east);
}

GeoRectangle GeoRectangle::BooleanUnion(const GeoRectangle& other) const {
  if (IsEmpty()) return other;

  if (other.IsEmpty()) return *this;

  const GeoCoordinates southWest(
      std::min(south_west_.GetLatitude(), other.south_west_.GetLatitude()),
      std::min(south_west_.GetLongitude(), other.south_west_.GetLongitude()));

  // Special handling to cover longitude wrapping
  double longitude1 = north_east_.GetLongitude();
  if (longitude1 < south_west_.GetLongitude()) longitude1 += math::two_pi;

  double longitude2 = other.north_east_.GetLongitude();
  if (longitude2 < other.south_west_.GetLongitude()) longitude2 += math::two_pi;

  double maxLongitude = std::max(longitude1, longitude2);
  if (maxLongitude > math::pi) {
    const double upperLimit =
        nextafter(southWest.GetLongitude(), southWest.GetLongitude() - 1.0);
    maxLongitude = std::min(maxLongitude - math::two_pi, upperLimit);
  }

  const GeoCoordinates northEast(
      std::max(north_east_.GetLatitude(), other.north_east_.GetLatitude()),
      maxLongitude);

  return GeoRectangle(southWest, northEast);
}

GeoRectangle& GeoRectangle::GrowToContain(const GeoCoordinates& point) {
  double pointLatitude = point.GetLatitude();
  double pointLongitude = point.GetLongitude();

  if (pointLatitude < south_west_.GetLatitude())
    south_west_.SetLatitude(pointLatitude);
  if (pointLatitude > north_east_.GetLatitude())
    north_east_.SetLatitude(pointLatitude);
  if (pointLongitude < south_west_.GetLongitude())
    south_west_.SetLongitude(pointLongitude);
  if (pointLongitude > north_east_.GetLongitude())
    north_east_.SetLongitude(pointLongitude);

  return *this;
}

}  // namespace geo
}  // namespace olp
