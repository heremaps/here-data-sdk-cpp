/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <map>
#include <string>
#include <vector>

namespace olp {
namespace client {
/**
 * @brief The API Services provided by the HERE platform.
 */
class Api {
 public:
  Api() = default;
  virtual ~Api() = default;

  /**
   * @brief Gets an API name.
   *
   * @return The API name.
   */
  const std::string& GetApi() const { return api_; }
  /**
   * @brief Gets a mutable reference to the API name.
   *
   * @return The mutable reference to the API name.
   */
  std::string& GetMutableApi() { return api_; }
  /**
   * @brief Sets the API name.
   *
   * @param value The API name.
   */
  void SetApi(const std::string& value) { this->api_ = value; }
  /**
   * @brief Gets a version of the API.
   *
   * @return The version of the API.
   */
  const std::string& GetVersion() const { return version_; }
  /**
   * @brief Gets a mutable reference to the API version.
   *
   * @return The mutable reference to the API version.
   */
  std::string& GetMutableVersion() { return version_; }
  /**
   * @brief Sets the API version.
   *
   * @param value The API version.
   */
  void SetVersion(const std::string& value) { this->version_ = value; }
  /**
   * @brief Gets an URL of the API.
   *
   * @return The URL of the API.
   */
  const std::string& GetBaseUrl() const { return baseUrl_; }
  /**
   * @brief Gets a mutable reference to the URL of the API.
   *
   * @return The mutable reference to the URL of the API.
   */
  std::string& GetMutableBaseUrl() { return baseUrl_; }
  /**
   * @brief Sets the URL of the API.
   *
   * @param value The URL of the API.
   */
  void SetBaseUrl(const std::string& value) { this->baseUrl_ = value; }
  /**
   * @brief Gets a map of parameters.
   *
   * @return The API parameters.
   */
  const std::map<std::string, std::string>& GetParameters() const {
    return parameters_;
  }
  /**
   * @brief Gets a mutable reference to the API parameters.
   *
   * @return The mutable reference to the API parameters.
   */
  std::map<std::string, std::string>& GetMutableParameters() {
    return parameters_;
  }
  /**
   * @brief Sets the API parameters.
   *
   * @param value The parameters map.
   */
  void SetParameters(const std::map<std::string, std::string>& value) {
    this->parameters_ = value;
  }

 private:
  std::string api_;
  std::string version_;
  std::string baseUrl_;
  std::map<std::string, std::string> parameters_;
};

/// An alias for the platform base URLs.
using Apis = std::vector<Api>;

}  // namespace client
}  // namespace olp
