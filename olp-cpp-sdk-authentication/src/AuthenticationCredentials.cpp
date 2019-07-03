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

#include "olp/authentication/AuthenticationCredentials.h"

namespace olp {
namespace authentication {
AuthenticationCredentials::AuthenticationCredentials(std::string key,
                                                     std::string secret)
    : key_(std::move(key)), secret_(std::move(secret)) {}

AuthenticationCredentials::AuthenticationCredentials(
    const AuthenticationCredentials& other)
    : key_(other.key_), secret_(other.secret_) {}

const std::string& AuthenticationCredentials::GetKey() const { return key_; }

const std::string& AuthenticationCredentials::GetSecret() const {
  return secret_;
}

}  // namespace authentication
}  // namespace olp
