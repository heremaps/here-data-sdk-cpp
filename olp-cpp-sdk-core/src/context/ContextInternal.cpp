/*
 * Copyright (C) 2023 HERE Europe B.V.
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

#include "ContextInternal.h"

#include <utility>

namespace olp {
namespace context {

std::shared_ptr<ContextData> Instance() {
  // Static initialization is thread safe.
  // Shared pointer allows to extend the life of ContextData
  // until all Scope objects are destroyed.
  static std::shared_ptr<ContextData> g_setup_vectors =
      std::make_shared<ContextData>();
  return g_setup_vectors;
}

}  // namespace context
}  // namespace olp
