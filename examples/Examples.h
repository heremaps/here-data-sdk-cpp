/*
 * Copyright (C) 2019-2026 HERE Europe B.V.
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

#include <olp/core/porting/export.h>

#ifdef EXAMPLES_SHARED_LIBRARY
#ifdef EXAMPLES_LIBRARY
#define EXAMPLES_API OLP_CPP_SDK_DECL_EXPORT
#else
#define EXAMPLES_API OLP_CPP_SDK_DECL_IMPORT
#endif  // EXAMPLES_LIBRARY
#else
#define EXAMPLES_API
#endif  // EXAMPLES_SHARED_LIBRARY

#include <string>

struct AccessKey {
  std::string id;      // Your here.access.key.id
  std::string secret;  // Your here.access.key.secret
};
