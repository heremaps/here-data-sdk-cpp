/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
#include <olp/core/geo/projection/IProjection.h>

namespace olp {
namespace geo {
/**
 * @brief The equirectangular (plate carree) projection.
 *
 * @see [Equirectangular projection](http://en.wikipedia.org/wiki/Equirectangular_projection)
 * on Wikipedia.
 *
 * The world and geographic coordinates are related in the following way:
 * - Latitude between -90 and 90 maps to the `Y` world axis of
 * the following range: [0, 0.5].
 * - Longitude between -180 and 180 maps to the `X` world axis of
 * the following range: [0, 1].
 * - Altitude maps to the `Z` world axis and is not scaled.
 */
class CORE_API EquirectangularProjection final : public IProjection {
 public:
  EquirectangularProjection() = default;
  ~EquirectangularProjection() override = default;

  GeoRectangle GetGeoBounds() const override;

  WorldAlignedBox WorldExtent(double minimum_altitude,
                              double maximum_altitude) const override;

  bool Project(const GeoCoordinates3d& geo_point,
               WorldCoordinates& world_point) const override;

  bool Unproject(const WorldCoordinates& world_point,
                 GeoCoordinates3d& geo_point) const override;
};
}  // namespace geo

}  // namespace olp
