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

#include <olp/core/geo/Types.h>

namespace olp {
namespace geo {

/**
 * @brief The identity projection used to work with geographic
 * and world coordinates. 
 */
class CORE_API IProjection {
 public:
  virtual ~IProjection() = default;

  /**
   * @brief Gets the geodetic bounds represented by the projection.
   *
   * @return The geodetic rectangle.
   */
  virtual GeoRectangle GetGeoBounds() const = 0;

  /**
   * @brief Creates the extent of world coordinates.
   * 
   * @param minimum_altitude The minimum altitude in meters.
   * @param maximum_altitude The maximum altitude in meters.
   *
   * @return The extent of world coordinates.
   */
  virtual WorldAlignedBox WorldExtent(double minimum_altitude,
                                      double maximum_altitude) const = 0;

  /**
   * @brief Checks whether the geographic coordinates (latitude, longitude, altitude)
   * of a point correspond to its world coordinates (x,y,z).
   * 
   * @param geo_point The point position in geographic coordinates.
   * @param world_point The point position in world coordinates.
   * 
   * @return True if the coordinates correspond; false otherwise.
   */
  virtual bool Project(const GeoCoordinates3d& geo_point,
                       WorldCoordinates& world_point) const = 0;

  /**
   * @brief Checks whether the world coordinates (x,y,z) of a point correspond to
   * its geographic coordinates (latitude, longitude, altitude).
   * 
   * @param world_point The position in world coordinates.
   * @param geo_point The position in geographic coordinates.
   * 
   * @return True if the coordinates correspond; false otherwise.
   */
  virtual bool Unproject(const WorldCoordinates& world_point,
                         GeoCoordinates3d& geo_point) const = 0;
};

}  // namespace geo
}  // namespace olp
