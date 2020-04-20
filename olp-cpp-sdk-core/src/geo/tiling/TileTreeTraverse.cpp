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

#include "olp/core/geo/tiling/TileTreeTraverse.h"

#include <cstdint>

#include "olp/core/geo/tiling/ISubdivisionScheme.h"
#include "olp/core/geo/tiling/SubTiles.h"
#include "olp/core/geo/tiling/TileKey.h"

namespace olp {
namespace geo {

TileTreeTraverse::TileTreeTraverse(
    const ISubdivisionScheme& sub_division_scheme)
    : sub_division_scheme_(sub_division_scheme) {}

TileTreeTraverse::NodeContainer TileTreeTraverse::SubNodes(
    const Node& node) const {
  const auto& subdivision = sub_division_scheme_.GetSubdivisionAt(node.Level());
  const unsigned sub_tile_count = subdivision.Width() * subdivision.Height();
  const uint16_t sub_tile_mask = ~(~0u << sub_tile_count);
  return NodeContainer(node, 1, sub_tile_mask);
}

}  // namespace geo
}  // namespace olp
