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
 * @brief Memory tracking definitions.
 *
 * If NETWORK_USE_MEMORY_LIB is non-zero then the memory library tracking
 * mechanism will be used.
 *
 * Otherwise the relevant macros, classes and type-aliases will be redefined to
 * have no effect thus eliminating memory library dependencies.
 */

#include <deque>
#include <map>
#include <memory>
#include <vector>

namespace olp {
namespace network {
namespace mem {
using std::deque;
using std::map;
using std::vector;

struct MemoryScopeTracker {
  MemoryScopeTracker() = default;
  explicit MemoryScopeTracker(bool) {}
  void Capture() {}
  void Clear() {}
};

}  // namespace mem
}  // namespace network
}  // namespace olp

#define MEMORY_TRACKER_SCOPE(tracker)

#define MEMORY_SCOPED_TAG(name)

#define MEMORY_GET_TAG_ALLOCATOR(name, type) \
  (type(type::key_compare(), std::allocator<type::value_type>()))
