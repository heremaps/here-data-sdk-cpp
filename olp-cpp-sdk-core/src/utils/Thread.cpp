/*
 * Copyright (C) 2024 HERE Europe B.V.
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

#include "olp/core/utils/Thread.h"
#include "olp/core/porting/platform.h"
#include "olp/core/utils/WarningWorkarounds.h"
#if defined(PORTING_PLATFORM_MAC)
#include <pthread.h>
#elif defined(PORTING_PLATFORM_ANDROID) || defined(PORTING_PLATFORM_LINUX) || \
    defined(PORTING_PLATFORM_QNX)
#include <pthread.h>
#endif

#include <cstring>
namespace olp {
namespace utils {

void Thread::SetCurrentThreadName(const std::string& thread_name) {
  // Currently only supported for pthread users
  OLP_SDK_CORE_UNUSED(thread_name);

#if defined(PORTING_PLATFORM_MAC)
  // Note that in Mac based systems the pthread_setname_np takes 1 argument
  // only.
  pthread_setname_np(thread_name.c_str());
#elif defined(PORTING_PLATFORM_ANDROID) || defined(PORTING_PLATFORM_LINUX) || \
    defined(PORTING_PLATFORM_QNX)
  // QNX allows 100 but Linux only 16 so select min value and apply for both.
  // If maximum length is exceeded on some systems, e.g. Linux, the name is not
  // set at all. So better truncate it to have at least the minimum set.
  constexpr size_t kMaxThreadNameLength = 16u;
  std::string truncated_name = thread_name.substr(0, kMaxThreadNameLength - 1);
  pthread_setname_np(pthread_self(), truncated_name.c_str());
#endif
}

}  // namespace utils
}  // namespace olp
