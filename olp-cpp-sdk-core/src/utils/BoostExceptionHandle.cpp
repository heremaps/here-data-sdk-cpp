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

#include <olp/core/logging/Log.h>
#include <olp/core/porting/export.h>

#if defined(OLP_SDK_BOOST_THROW_EXCEPTION)

#include <stdio.h>
#include <stdlib.h>
#include <exception>

namespace {
constexpr auto kLogTag = "BoostExceptionHandle";
}

namespace boost {

OLP_CPP_SDK_DECL_EXPORT void throw_exception(const std::exception& e) {
  OLP_SDK_LOG_ABORT_F(kLogTag, "Exception occurred: '%s'", e.what());
}

}  // namespace boost
#endif
