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

#include <functional>
#include <memory>
#include <string>

namespace olp {
namespace authentication {
/**
 * @brief Authetication Utils
 */
class AuthenticationUtils {
 public:
  struct DeleteUserResponse {
    int status;
    std::string error;
  };

  AuthenticationUtils();
  virtual ~AuthenticationUtils();

  typedef std::function<void(const DeleteUserResponse& response)> UserCallback;

  void deleteHereUser(const std::string& user_bearer_token,
                      const UserCallback& callback);

 private:
  class Impl;
  std::unique_ptr<Impl> d;
};

}  // namespace authentication
}  // namespace olp
