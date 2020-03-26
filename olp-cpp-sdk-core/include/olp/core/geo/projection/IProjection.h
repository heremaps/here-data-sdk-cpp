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

class CORE_API IProjection {
 public:
  virtual ~IProjection() = default;

  /**
   * @brief Get geodetic bounds representable by projection.
   *
   * @return Geodetic rectangle
   */
  virtual GeoRectangle GetGeoBounds() const = 0;

  /**
   * @brief Extent of world coordinates.
   */
  virtual WorldAlignedBox WorldExtent(double minimum_altitude,
                                      double maximum_altitude) const = 0;

  virtual bool Project(const GeoCoordinates3d& geo_point,
                       WorldCoordinates& world_point) const = 0;

  virtual bool Unproject(const WorldCoordinates& world_point,
                         GeoCoordinates3d& geo_point) const = 0;
};

}  // namespace geo
}  // namespace olp
