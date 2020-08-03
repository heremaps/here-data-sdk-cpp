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

#include <ctime>
#include <memory>
#include <string>

#include <rapidjson/document.h>

#include "BaseResult.h"

namespace olp {
namespace authentication {
class SignUpResultImpl : public BaseResult {
 public:
  SignUpResultImpl() noexcept;

  SignUpResultImpl(
      int status, std::string error,
      std::shared_ptr<rapidjson::Document> json_document = nullptr) noexcept;

  ~SignUpResultImpl() override;

  /**
   * @brief error Here account user identifier
   * @return error string cotaining the Here account user identifier
   */
  const std::string& GetUserIdentifier() const;

 private:
  std::string user_identifier_;
};
}  // namespace authentication
}  // namespace olp
