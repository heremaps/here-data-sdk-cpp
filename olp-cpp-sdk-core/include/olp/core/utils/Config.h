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
 * @file Config.h
 * @brief Contains macros that determine the compile-time configuration.
 */

/// Set to 1 if the exceptions are enabled; 0 if disabled.
#define CORE_EXCEPTIONS_ENABLED 0

#if defined(__EXCEPTIONS) || defined(_CPPUNWIND) || defined(__CPPUNWIND)
#undef CORE_EXCEPTIONS_ENABLED
#define CORE_EXCEPTIONS_ENABLED 1
#endif

/// Set to 1 if C++ RTTI is enabled; 0 if disabled.
#define CORE_RTTI_ENABLED 0

#if defined(__GNUC__)
#ifdef __GXX_RTTI
#undef CORE_RTTI_ENABLED
#define CORE_RTTI_ENABLED 1
#endif
#elif defined(__clang__)
#if __has_feature(cxx_rtti)
#undef CORE_RTTI_ENABLED
#define CORE_RTTI_ENABLED 1
#endif
#elif defined(_MSC_VER)
#ifdef _CPPRTTI
#undef CORE_RTTI_ENABLED
#define CORE_RTTI_ENABLED 1
#endif
#endif
