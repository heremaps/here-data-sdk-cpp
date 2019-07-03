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

#include <olp/core/geo/Types.h>

namespace olp {
namespace geo {
class CORE_API TileTreeTraverse {
 public:
  using Node = TileKey;
  using NodeContainer = SubTiles;

  TileTreeTraverse(const ISubdivisionScheme& sub_division_scheme);

  NodeContainer SubNodes(const Node& node) const;

 private:
  const ISubdivisionScheme& sub_division_scheme_;
};

}  // namespace geo
}  // namespace olp
