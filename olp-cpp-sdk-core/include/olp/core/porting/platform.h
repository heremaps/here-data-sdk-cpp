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

#if defined(EMSCRIPTEN)
#define PORTING_PLATFORM_EMSCRIPTEN
#elif defined(__ANDROID__)
#define PORTING_PLATFORM_ANDROID
#elif defined(__linux)
#define PORTING_PLATFORM_LINUX
#elif defined(_WIN32)
#define PORTING_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PORTING_PLATFORM_MAC
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define PORTING_PLATFORM_IOS
#endif
#elif defined(__QNX__)
#define PORTING_PLATFORM_QNX
#else
#warning Cannot determine platform
#endif
