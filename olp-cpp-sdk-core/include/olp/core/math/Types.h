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

namespace olp {
namespace math {

template <typename T>
struct Vector3;
using Vector3d = Vector3<double>;

// AlignedBox
template <typename T, unsigned int N>
class AlignedBox;

template <typename T>
using AlignedBox3 = AlignedBox<T, 3>;
using AlignedBox3d = AlignedBox<double, 3>;

// Size
template <typename T>
class Size2;
using Size2u = Size2<unsigned>;

}  // namespace math
}  // namespace olp
