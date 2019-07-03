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

#include "ThreadId.h"
#include <olp/core/porting/platform.h>

#if defined(PORTING_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(PORTING_PLATFORM_LINUX) || defined(PORTING_PLATFORM_ANDROID)
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(PORTING_PLATFORM_QNX)
#include <process.h>
#elif defined(PORTING_PLATFORM_MAC)
#include <pthread.h>
#include <stdint.h>
#elif !defined(PORTING_PLATFORM_EMSCRIPTEN)
#include <functional>
#include <thread>
#endif

namespace olp {
namespace logging {
unsigned long getThreadId() {
#if defined(PORTING_PLATFORM_WINDOWS)
  return GetCurrentThreadId();
#elif defined(PORTING_PLATFORM_ANDROID) || defined(PORTING_PLATFORM_QNX)
  return static_cast<unsigned long>(gettid());
#elif defined(PORTING_PLATFORM_LINUX)
  return static_cast<unsigned long>(syscall(SYS_gettid));
#elif defined(PORTING_PLATFORM_MAC)
  uint64_t threadId;
  pthread_threadid_np(pthread_self(), &threadId);
  return static_cast<unsigned long>(threadId);
#elif defined(PORTING_PLATFORM_EMSCRIPTEN)
  return 0;
#else
  // Generic fallback
  return static_cast<unsigned long>(
      std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
}

}  // namespace logging
}  // namespace olp
