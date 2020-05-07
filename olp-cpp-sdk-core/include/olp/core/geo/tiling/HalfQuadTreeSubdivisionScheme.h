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

#include <olp/core/geo/tiling/ISubdivisionScheme.h>

namespace olp {
namespace geo {

/**
 * @brief Half-quadtree subdivision scheme.
 *
 * Subdivides 0-th level tile into left and right parts; on other levels same as
 * quadtree.
 */
class CORE_API HalfQuadTreeSubdivisionScheme final : public ISubdivisionScheme {
 public:
  HalfQuadTreeSubdivisionScheme() = default;
  ~HalfQuadTreeSubdivisionScheme() override = default;

  bool IsEqualTo(const ISubdivisionScheme& other) const override;

  const std::string& GetName() const override;

  math::Size2u GetSubdivisionAt(unsigned level) const override;

  math::Size2u GetLevelSize(unsigned level) const override;
};

}  // namespace geo
}  // namespace olp
