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

#if defined(_MSC_VER)
#define OLP_CPP_SDK_DECL_EXPORT __declspec(dllexport)
#define OLP_CPP_SDK_DECL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
#define OLP_CPP_SDK_DECL_EXPORT __attribute__((visibility("default")))
#define OLP_CPP_SDK_DECL_IMPORT
#else
#define OLP_CPP_SDK_DECL_EXPORT
#define OLP_CPP_SDK_DECL_IMPORT
#endif  // _MSC_VER
