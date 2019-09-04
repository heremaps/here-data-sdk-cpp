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

#include "BaseResult.h"

namespace olp {
namespace authentication {
class SignOutResultImpl : public BaseResult {
 public:
  SignOutResultImpl() noexcept;

  SignOutResultImpl(
      int status, std::string error,
      std::shared_ptr<rapidjson::Document> json_document = nullptr) noexcept;

  ~SignOutResultImpl() override;
};
}  // namespace authentication
}  // namespace olp
