/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <string>

namespace helpers {
/**
 * @brief Makes directories content readonly or read-write
 *
 * @param path Path to the directory.
 * @param readonly Readonly if true, read-write if false
 *
 * @return \c true if any file with the given path exists, \c false otherwise.
 */
bool MakeDirectoryContentReadonly(const std::string& path, bool readonly);
}  // namespace helpers
