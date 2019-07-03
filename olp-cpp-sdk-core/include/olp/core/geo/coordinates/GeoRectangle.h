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
#include <olp/core/geo/coordinates/GeoCoordinates.h>

namespace olp {
namespace geo {
/**
 * @brief A rectangular area in WGS84 coordinates
 *
 * @sa GeoBox adds an altitude
 */
class CORE_API GeoRectangle {
 public:
  /** Constructs an empty rectangle. */
  GeoRectangle();

  GeoRectangle(const GeoCoordinates& south_west,
               const GeoCoordinates& north_east);

  GeoCoordinates SouthEast() const;

  GeoCoordinates SouthWest() const;

  GeoCoordinates NorthEast() const;

  GeoCoordinates NorthWest() const;

  GeoCoordinates Center() const;

  bool IsEmpty() const;

  double LongitudeSpan() const;

  double LatitudeSpan() const;

  bool Contains(const GeoCoordinates& point) const;

  bool Overlaps(const GeoRectangle& rect) const;

  bool operator==(const GeoRectangle& other) const;

  bool operator!=(const GeoRectangle& other) const;

  GeoRectangle BooleanUnion(const GeoRectangle& other) const;

  GeoRectangle& GrowToContain(const GeoCoordinates& point);

 protected:
  GeoCoordinates south_west_;
  GeoCoordinates north_east_;
};

}  // namespace geo
}  // namespace olp
