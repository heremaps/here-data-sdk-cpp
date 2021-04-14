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

#include <olp/core/geo/tiling/ITilingScheme.h>

namespace olp {
namespace geo {

/**
 * @brief Represents how data is tiled.
 *
 * @tparam TSubdivisionScheme The subdivision scheme.
 * @tparam TProjection The identity projection.
 */
template <class TSubdivisionScheme, class TProjection>
class TilingScheme : public ITilingScheme {
 public:
  /// An alias for the subdivision scheme used by this tiling scheme.
  using SubdivisionScheme = TSubdivisionScheme;
  /// An alias for the projection used by this tiling scheme.
  using Projection = TProjection;

  TilingScheme() = default;
  ~TilingScheme() override = default;

  /// @copydoc ITilingScheme::GetSubdivisionScheme()
  const ISubdivisionScheme& GetSubdivisionScheme() const override {
    return subdivision_scheme_;
  }

  /// @copydoc ITilingScheme::GetProjection()
  const IProjection& GetProjection() const override { return projection_; }

 private:
  SubdivisionScheme subdivision_scheme_{};
  Projection projection_{};
};

}  // namespace geo
}  // namespace olp
