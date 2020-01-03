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

#include "olp/authentication/SignInResult.h"

#include "SignInResultImpl.h"

using namespace rapidjson;

namespace olp {
namespace authentication {
SignInResult::SignInResult() noexcept
    : impl_(std::make_shared<SignInResultImpl>()) {}

SignInResult::~SignInResult() = default;

int SignInResult::GetStatus() const { return impl_->GetStatus(); }

const ErrorResponse& SignInResult::GetErrorResponse() const {
  return impl_->GetErrorResponse();
}

const ErrorFields& SignInResult::GetErrorFields() const {
  return impl_->GetErrorFields();
}

const std::string& SignInResult::GetAccessToken() const {
  return impl_->GetAccessToken();
}

const std::string& SignInResult::GetClientId() const {
  return impl_->GetClientId();
}

const std::string& SignInResult::GetTokenType() const {
  return impl_->GetTokenType();
}

const std::string& SignInResult::GetRefreshToken() const {
  return impl_->GetRefreshToken();
}

time_t SignInResult::GetExpiryTime() const { return impl_->GetExpiryTime(); }

const std::string& SignInResult::GetUserIdentifier() const {
  return impl_->GetUserIdentifier();
}

const std::string& SignInResult::GetScope() const { return impl_->GetScope(); }

SignInResult::SignInResult(std::shared_ptr<SignInResultImpl> impl)
    : impl_(impl) {}

}  // namespace authentication
}  // namespace olp
