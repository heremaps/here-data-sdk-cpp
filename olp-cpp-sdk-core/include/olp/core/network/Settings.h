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
namespace network {
/**
 * @brief Settings class provides access to configuration settings
 *
 * Support settings values are:
 *   HYPE_METRICS_LOGGING - Enable logging of the runtime metrics events
 *   HYPE_METRICS_SUMMARY - Log summary of the metrics in the end
 */
class CORE_API Settings {
 public:
  /**
   * @brief returns the settings value for the specified key or a
   * default value
   * @param key is the settings name
   * @param default_value is the default value to be returned in case
   * the value is not found with the key
   * @return the settings value for this key
   */
  static const std::string GetEnvString(
      const char* key, const std::string& default_value = std::string());

  /**
   * @brief returns an integer settings value for the specified key or a
   * default value
   * @param key is the settings name
   * @param default_value is the default value to be returned in case
   * the value is not found with the key
   * @return the settings value for this key
   */
  static int GetEnvInt(const char* key, int default_value);

  /**
   * @brief Set string setting
   * @param key is the settings name
   * @param value of the settings
   * @return old value or empty string if it was not set
   */
  static const std::string SetValue(std::string key, std::string value);
};

}  // namespace network
}  // namespace olp
