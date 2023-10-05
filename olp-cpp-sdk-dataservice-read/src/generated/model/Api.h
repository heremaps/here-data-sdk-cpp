/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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
namespace dataservice {
namespace read {
/**
 * @brief model namespace
 */
namespace model {
/**
 * @brief API services provided by the HERE platform.
 */
class Api {
 public:
  Api() = default;
  Api(const Api&) = default;
  Api(Api&&) = default;
  Api& operator=(const Api&) = default;
  Api& operator=(Api&&) = default;
  virtual ~Api() = default;

 private:
  std::string api_;
  std::string version_;
  std::string baseUrl_;
  std::map<std::string, std::string> parameters_;

 public:
  const std::string& GetApi() const { return api_; }
  std::string& GetMutableApi() { return api_; }
  void SetApi(const std::string& value) { this->api_ = value; }

  const std::string& GetVersion() const { return version_; }
  std::string& GetMutableVersion() { return version_; }
  void SetVersion(const std::string& value) { this->version_ = value; }

  const std::string& GetBaseUrl() const { return baseUrl_; }
  std::string& GetMutableBaseUrl() { return baseUrl_; }
  void SetBaseUrl(const std::string& value) { this->baseUrl_ = value; }

  const std::map<std::string, std::string>& GetParameters() const {
    return parameters_;
  }
  std::map<std::string, std::string>& GetMutableParameters() {
    return parameters_;
  }
  void SetParameters(const std::map<std::string, std::string>& value) {
    this->parameters_ = value;
  }
};

/// Alias for the platform base URLs.
using Apis = std::vector<Api>;

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
