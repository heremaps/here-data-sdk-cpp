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

#include <functional>
#include <string>

#include <olp/core/CoreApi.h>

namespace olp {
namespace utils {

class CORE_API Dir {
 public:
  /// Alias for filter function.
  using FilterFunction = std::function<bool(const std::string&)>;

  /**
   * @brief Checks if directory exists.
   *
   * @param path Path of the directory.
   *
   * @return \c true if directory exists, \c false otherwise.
   *
   * @deprecated Will be removed by 10.2020. Please use Exists() instead.
   */
  static bool exists(const std::string& path);

  /**
   * @brief Checks if directory exists.
   *
   * @param path Path of the directory.
   *
   * @return true if directory exists, false otherwise.
   */
  static bool Exists(const std::string& path);

  /**
   * @brief Removes the directory, deleting all subfolders and files.
   *
   * @param path Path of the directory.
   *
   * @return true if operation is successfull, false otherwise.
   *
   *  @deprecated Will be removed by 10.2020. Please use Remove() instead.
   */
  static bool remove(const std::string& path);

  /**
   * @brief Removes the directory, deleting all subfolders and files.
   *
   * @param path Path of the directory.
   *
   * @return \c true if operation is successfull, \c false otherwise.
   */
  static bool Remove(const std::string& path);

  /**
   * @brief Creates the directory including all required directories on
   * the path.
   *
   * @param path Path of the directory.
   *
   * @return \c true if operation is successfull, \c false otherwise.
   *
   * @deprecated Will be removed by 10.2020. Please use Create() instead.
   */
  static bool create(const std::string& path);

  /**
   * @brief Creates the directory including all required directories on
   * the path.
   *
   * @param path Path of the directory.
   *
   * @return \c true if operation is successfull, \c false otherwise.
   */
  static bool Create(const std::string& path);

  /**
   * @brief Returns the platform-specific temporary path.
   *
   * @return The platform specific temporary directory path.
   */
  static std::string TempDirectory();

  /**
   * @brief Check whether the provided file exists.
   *
   * @param file_path Path to the file.
   *
   * @return \c true if any file with the given path exists, \c false otherwise.
   */
  static bool FileExists(const std::string& file_path);

  /**
   * @brief Calculates size of the directory. Filter may be applied to exclude
   * unnecessary files or directories from calculation.
   *
   * @param path Path of the directory.
   * @param filter_fn Filter function.
   *
   * @return Calculated size.
   */
  static uint64_t Size(const std::string& path, FilterFunction filter_fn = {});
};

}  // namespace utils
}  // namespace olp
