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

#include <olp/core/geo/tiling/ITilingScheme.h>

namespace olp {
namespace geo {

template <class TSubdivisionScheme, class TProjection>
class TilingScheme : public ITilingScheme {
 public:
  using SubdivisionScheme = TSubdivisionScheme;
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
