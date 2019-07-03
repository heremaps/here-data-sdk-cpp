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

#include <olp/core/geo/tiling/QuadTreeSubdivisionScheme.h>

namespace olp {
namespace geo {
bool QuadTreeSubdivisionScheme::IsEqualTo(
    const ISubdivisionScheme& other) const {
  return GetName() == other.GetName();
}

const std::string& QuadTreeSubdivisionScheme::GetName() const {
  static const std::string name{"QuadTreeSubdivisionScheme"};
  return name;
}

math::Size2u QuadTreeSubdivisionScheme::GetSubdivisionAt(
    unsigned /*level*/) const {
  return {2u, 2u};
}

math::Size2u QuadTreeSubdivisionScheme::GetLevelSize(unsigned level) const {
  return {1u << level, 1u << level};
}

}  // namespace geo
}  // namespace olp
