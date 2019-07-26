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

/**
 * @file
 * Contains utilities to work around compiler warnings.
 */

/**
 * Marks arbitrary many variables as unused to avoid compiler warnings.
 */
#define CORE_UNUSED(...) olp::core::Unused(__VA_ARGS__)

namespace olp {
namespace core {
template <typename... Args>
void Unused(Args&&...) {}
}  // namespace core
}  // namespace olp

/**
 * While statement for "do {} while (0)" used for macros that bypasses compiler
 * warnings.
 */
#ifdef _MSC_VER
#define CORE_LOOP_ONCE()                                                  \
  __pragma(warning(push)) __pragma(warning(disable : 4127)) while (false) \
      __pragma(warning(pop))
#else
#define CORE_LOOP_ONCE() while (false)
#endif
