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
#include <olp/core/math/Types.h>
#include <olp/core/math/Vector.h>

namespace olp {
namespace geo {
class EquirectangularProjection;
class GeoCoordinates3d;
class GeoCoordinates;
class GeoPoint;
class GeoRectangle;
class HalfQuadTreeSubdivisionScheme;
class IProjection;
class ISubdivisionScheme;
class ITilingScheme;
class IdentityProjection;
class QuadTreeSubdivisionScheme;
class SphereProjection;
class SubTiles;
class TileKey;
class TileTreeTraverse;
class TilingSchemeRegistry;
class WebMercatorProjection;

using WorldAlignedBox = math::AlignedBox3d;
using WorldCoordinates = math::Vector3d;

struct CORE_API DegreeType {};

}  // namespace geo
}  // namespace olp
