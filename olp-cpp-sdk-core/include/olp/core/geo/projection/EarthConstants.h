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

namespace olp {
namespace geo {
/**
 * @brief Various WGS84 Earth's constants.
 */
class CORE_API EarthConstants {
 public:
  /**
   * @brief The Earth's equatorial radius is the distance from its center to the equator.
   * @see https://en.wikipedia.org/wiki/Earth_radius
   * @return Earth's equatorial radius.
   */
  static double EquatorialRadius();
  /**
   * @brief The Earth's polar radius is the distance from its center to the North and South Poles.
   * @see https://en.wikipedia.org/wiki/Earth_radius
   * @return Earth's polar radius.
   */
  static double PolarRadius();
  /**
   * @brief The Earth's circumference is the distance around the Earth equator.
   * @see https://en.wikipedia.org/wiki/Earth%27s_circumference
   * @return Earth's circumference.
   */
  static double EquatorialCircumference();

  /**
   * @brief Lowest, natural dry-land elevation
   *
   * Area surrounding the Dead Sea in Israel.
   *
   * Measured in meters relative to MSL.
   *
   * @return elevation value.
   */
  static double MinimumElevation();

  /**
   * @brief Highest natural dry-land elevation
   *
   * Peak of Mount Everest.
   *
   * Measured in meters relative to MSL.
   *
   * @return elevation value.
   */
  static double MaximumElevation();
};

inline double EarthConstants::EquatorialRadius() { return 6378137.0; }

inline double EarthConstants::PolarRadius() { return 6356752.3142; }

inline double EarthConstants::EquatorialCircumference() {
  // 2.0 * equatorialRadius() * math::pi<double>()
  return 40075016.6855784861531768177614;
}

inline double EarthConstants::MinimumElevation() { return -418; }

inline double EarthConstants::MaximumElevation() { return 8848; }

}  // namespace geo
}  // namespace olp
