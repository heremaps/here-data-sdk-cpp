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

// MSVC Specific
#if defined( _MSC_VER )
#define PORTING_MSVC_DISABLE_WARNINGS( a ) __pragma( warning( disable : a ) )

#define PORTING_MSVC_PUSH_WARNINGS( ) __pragma( warning( push ) )

#define PORTING_MSVC_POP_WARNINGS( ) __pragma( warning( pop ) )

#else
#define PORTING_MSVC_DISABLE_WARNINGS( a )
#define PORTING_MSVC_PUSH_WARNINGS( )
#define PORTING_MSVC_POP_WARNINGS( )
#endif

// Needed for concating args for clang and gcc
#if defined( __GNUC__ )
#define PORTING_DO_PRAGMA_( x ) _Pragma( #x )
#define PORTING_DO_PRAGMA( x ) PORTING_DO_PRAGMA_( x )
#endif

// CLANG Specific
#if defined( __clang__ )
#define PORTING_CLANG_DISABLE_WARNING( arg ) PORTING_DO_PRAGMA( clang diagnostic ignored arg )

#define PORTING_CLANG_PUSH_WARNINGS( ) PORTING_DO_PRAGMA( clang diagnostic push )

#define PORTING_CLANG_POP_WARNINGS( ) PORTING_DO_PRAGMA( clang diagnostic pop )

#else
#define PORTING_CLANG_DISABLE_WARNING( a )
#define PORTING_CLANG_PUSH_WARNINGS( )
#define PORTING_CLANG_POP_WARNINGS( )
#endif

// GNU Specific
#if defined( __GNUC__ ) && !defined( __clang__ )
#define PORTING_GCC_DISABLE_WARNING( arg ) PORTING_DO_PRAGMA( GCC diagnostic ignored arg )

#define PORTING_GCC_PUSH_WARNINGS( ) PORTING_DO_PRAGMA( GCC diagnostic push )

#define PORTING_GCC_POP_WARNINGS( ) PORTING_DO_PRAGMA( GCC diagnostic pop )

#else
#define PORTING_GCC_DISABLE_WARNING( a )
#define PORTING_GCC_PUSH_WARNINGS( )
#define PORTING_GCC_POP_WARNINGS( )
#endif

// for both gcc and clang
#if defined( __GNUC__ )
#define PORTING_CLANG_GCC_DISABLE_WARNING( arg ) PORTING_DO_PRAGMA( GCC diagnostic ignored arg )
#else
#define PORTING_CLANG_GCC_DISABLE_WARNING( a )
#endif

// helper state maintaining macros
#define PORTING_PUSH_WARNINGS( )   \
    PORTING_CLANG_PUSH_WARNINGS( ) \
    PORTING_GCC_PUSH_WARNINGS( )   \
    PORTING_MSVC_PUSH_WARNINGS( )

#define PORTING_POP_WARNINGS( )   \
    PORTING_CLANG_POP_WARNINGS( ) \
    PORTING_GCC_POP_WARNINGS( )   \
    PORTING_MSVC_POP_WARNINGS( )
