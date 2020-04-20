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

#include <string>

#include <olp/core/geo/Types.h>
#include <olp/core/math/Size.h>

namespace olp {
namespace geo {

/**
 * Abstract tiling subdivision scheme.
 */
class CORE_API ISubdivisionScheme {
 public:
  virtual ~ISubdivisionScheme() = default;

  /**
   * Check whether two schemes are equal.
   *
   * @param[in] other Other scheme.
   *
   * @return True if equal, false otherwise.
   */
  virtual bool IsEqualTo(const ISubdivisionScheme& other) const = 0;

  /**
   * Get unique scheme name.
   *
   * @return Scheme name.
   */
  virtual const std::string& GetName() const = 0;

  /**
   * Get number of subtiles a tile splits into at given level.
   *
   * @param[in] level Subdivision level.
   *
   * @return Horizontal and vertical number of subtiles (e.g. 2x2).
   */
  virtual math::Size2u GetSubdivisionAt(unsigned level) const = 0;

  /**
   * Get size of a given level.
   *
   * @param[in] level Subdivision level.
   *
   * @return Horizontal and vertical number of tiles (e.g. 2^level x 2^level).
   */
  virtual math::Size2u GetLevelSize(unsigned level) const = 0;
};

}  // namespace geo
}  // namespace olp
