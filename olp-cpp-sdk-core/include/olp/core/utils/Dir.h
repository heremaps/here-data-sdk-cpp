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

#include <string>

#include <olp/core/CoreApi.h>

namespace olp {
namespace utils {

class CORE_API Dir {
 public:
  /**
   * @brief exists checks if directory exists
   * @param path path of the directory
   * @return true if directory exists, false otherwise.
   */
  static bool exists(const std::string& path);

  /**
   * @brief remove removes the directory, deleting all subfolders and files.
   * @param path path of the directory
   * @return true if operation is successfull, false otherwise.
   */
  static bool remove(const std::string& path);

  /**
   * @brief create creates the directory including all required directories on
   * the path.
   * @param path path of the directory
   * @return true if operation is successfull, false otherwise.
   */
  static bool create(const std::string& path);

  /**
   * @brief TempDirectory returns the platform-specific temporary path.
   * @return temporary directory path
   */
  static std::string TempDirectory();

  /**
   * @brief Check whether file exists.
   * @param[in] file_path Path to the file.
   * @return \c true if any file with the given path exists, \c false otherwise.
   */
  static bool FileExists(const std::string& file_path);
};

}  // namespace utils
}  // namespace olp
