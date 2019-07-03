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

#include <olp/core/geo/projection/EquirectangularProjection.h>
#include <olp/core/geo/projection/IdentityProjection.h>
#include <olp/core/geo/projection/WebMercatorProjection.h>
#include <olp/core/geo/tiling/HalfQuadTreeSubdivisionScheme.h>
#include <olp/core/geo/tiling/QuadTreeSubdivisionScheme.h>
#include <olp/core/geo/tiling/TilingScheme.h>

namespace olp {
namespace geo {
using HalfQuadTreeIdentityTilingScheme =
    TilingScheme<HalfQuadTreeSubdivisionScheme, IdentityProjection>;
using HalfQuadTreeEquirectangularTilingScheme =
    TilingScheme<HalfQuadTreeSubdivisionScheme, EquirectangularProjection>;
using HalfQuadTreeMercatorTilingScheme =
    TilingScheme<HalfQuadTreeSubdivisionScheme, WebMercatorProjection>;
using QuadTreeIdentityTilingScheme =
    TilingScheme<QuadTreeSubdivisionScheme, IdentityProjection>;
using QuadTreeEquirectangularTilingScheme =
    TilingScheme<QuadTreeSubdivisionScheme, EquirectangularProjection>;
using QuadTreeMercatorTilingScheme =
    TilingScheme<QuadTreeSubdivisionScheme, WebMercatorProjection>;
}  // namespace geo
}  // namespace olp
