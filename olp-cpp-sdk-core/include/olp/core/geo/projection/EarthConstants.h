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
 * @brief Represents WGS84 Earth's constants.
 */
class CORE_API EarthConstants {
 public:
  /**
   * @brief Gets the Earth's equatorial radius, which is the distance from
   * its center to the equator.
   *
   * @see [Earth radius](https://en.wikipedia.org/wiki/Earth_radius) on Wikipedia.
   *
   * @return The Earth's equatorial radius.
   */
  static double EquatorialRadius();
  /**
   * @brief Gets the Earth's polar radius, which is the distance from
   * its center to the North and South Poles.
   *
   * @see [Polar radius](https://en.wikipedia.org/wiki/Earth_radius#Polar_radius)
   * on Wikipedia.
   *
   * @return The Earth's polar radius.
   */
  static double PolarRadius();
  /**
   * @brief Gets the Earth's circumference, which is the distance
   * around the Earth equator.
   *
   * @see [Earth circumference](https://en.wikipedia.org/wiki/Earth%27s_circumference)
   * on Wikipedia.
   *
   * @return The Earth's circumference.
   */
  static double EquatorialCircumference();

  /**
   * @brief Gets the lowest natural dry-land elevation.
   *
   * The area surrounding the Dead Sea in Israel.
   * Measured in meters relative to MSL.
   *
   * @return The elevation value.
   */
  static double MinimumElevation();

  /**
   * @brief Gets the highest natural dry-land elevation.
   *
   * The peak of Mount Everest.
   * Measured in meters relative to MSL.
   *
   * @return The elevation value.
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
