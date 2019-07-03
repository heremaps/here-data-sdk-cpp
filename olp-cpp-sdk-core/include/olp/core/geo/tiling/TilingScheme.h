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
#include <olp/core/utils/TypeId.h>

namespace olp {
namespace geo {
template <class TSubdivisionScheme, class TProjection>
class TilingScheme : public ITilingScheme {
 public:
  CORE_DEFINE_RTTI

  using SubdivisionScheme = TSubdivisionScheme;
  using Projection = TProjection;

  TilingScheme() {}

  // ITilingScheme
  bool IsEqualTo(const ITilingScheme& other) const override;
  const ISubdivisionScheme& GetSubdivisionScheme() const override;
  const IProjection& GetProjection() const override;

 private:
  SubdivisionScheme sub_division_scheme_{};
  Projection projection_{};
};

template <class S, class P>
inline bool TilingScheme<S, P>::IsEqualTo(const ITilingScheme& other) const {
  return core::typeId(*this) == core::typeId(other);
}

template <class S, class P>
inline const ISubdivisionScheme& TilingScheme<S, P>::GetSubdivisionScheme()
    const {
  return sub_division_scheme_;
}

template <class S, class P>
inline const IProjection& TilingScheme<S, P>::GetProjection() const {
  return projection_;
}

}  // namespace geo
}  // namespace olp
