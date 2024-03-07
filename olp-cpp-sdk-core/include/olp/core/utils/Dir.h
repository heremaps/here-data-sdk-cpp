/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <cstdint>
#include <functional>
#include <string>

#include <olp/core/CoreApi.h>

namespace olp {
namespace utils {

/** @brief Manages directories.
 */
class CORE_API Dir {
 public:
  /// An alias for the filter function.
  using FilterFunction = std::function<bool(const std::string&)>;

  /// An alias for the path callback function.
  using PathCallback = std::function<void(const std::string&)>;

  /**
   * @brief Checks whether a directory exists.
   *
   * @param path The path of the directory.
   *
   * @return True if the directory exists; false otherwise.
   *
   * @deprecated Will be removed by 10/2020. Use `Exists()` instead.
   */
  static bool exists(const std::string& path);

  /**
   * @brief Checks whether a directory exists.
   *
   * @param path The path of the directory.
   *
   * @return True if the directory exists; false otherwise.
   */
  static bool Exists(const std::string& path);

  /**
   * @brief Removes a directory and deletes all its subfolders and files.
   *
   * @param path The path of the directory.
   *
   * @return True if the operation is successful; false otherwise.
   *
   * @deprecated Will be removed by 10/2020. Use `Remove()` instead.
   */
  static bool remove(const std::string& path);

  /**
   * @brief Removes a directory and deletes all its subfolders and files.
   *
   * @param path The path of the directory.
   *
   * @return True if the operation is successful; false otherwise.
   */
  static bool Remove(const std::string& path);

  /**
   * @brief Creates a directory and all required directories specified
   * in the path.
   *
   * @param path The path of the directory.
   *
   * @return True if the operation is successful; false otherwise.
   *
   * @deprecated Will be removed by 10/2020. Use `Create()` instead.
   */
  static bool create(const std::string& path);

  /**
   * @brief Creates a directory and all required directories specified
   * in the path.
   *
   * @param path The path of the directory.
   * @param extend_permissions Controls the permission extension.
   *
   * Note: extend_permissions are applicable for Unix systems, if enabled, it
   * sets the 0777 permissions for created folders.
   *
   * @return True if the operation is successful; false otherwise.
   */
  static bool Create(const std::string& path, bool extend_permissions = false);

  /**
   * @brief Gets a platform-specific temporary path.
   *
   * @return The platform-specific temporary path of the directory.
   */
  static std::string TempDirectory();

  /**
   * @brief Checks whether the provided file exists.
   *
   * @param file_path The path of the file.
   *
   * @return True if the file with the given path exists; false otherwise.
   */
  static bool FileExists(const std::string& file_path);

  /**
   * @brief Calculates the size of a directory.
   *
   * Use a filter to exclude unnecessary files or directories from the
   * calculation.
   *
   * @note This method will go all the way recursive for as long as needed
   * to gather all files which pass the given filter.
   *
   * @param path The path of the directory.
   * @param filter_fn The filter function.
   *
   * @return The calculated size.
   */
  static uint64_t Size(const std::string& path, FilterFunction filter_fn = {});

  /**
   * @brief Checks if the current application and user has a read only access to
   * given path.
   *
   * @param path The path to check for read only access.
   *
   * @return True if current application and user has read only access to the
   * path.
   */
  static bool IsReadOnly(const std::string& path);

  /**
   * @brief Iterates through top-level directories in the provided path and
   * calls the provided callback function for each directory.
   *
   * @param path The path of the root directory.
   * @param path_fn The callback function with a relative path.
   */
  static void ForEachDirectory(const std::string& path, PathCallback path_fn);
};

}  // namespace utils
}  // namespace olp
