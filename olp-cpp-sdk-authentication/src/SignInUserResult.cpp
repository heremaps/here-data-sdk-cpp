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

#include "olp/authentication/SignInUserResult.h"

#include "Constants.h"
#include "SignInUserResultImpl.h"
#include "olp/authentication/SignInResult.h"

namespace olp {
namespace authentication {
SignInUserResult::~SignInUserResult() = default;

const std::string& SignInUserResult::GetTermAcceptanceToken() const {
  return impl_->GetTermAcceptanceToken();
}

const std::string& SignInUserResult::GetTermsOfServiceUrl() const {
  return impl_->GetTermsOfServiceUrl();
}

const std::string& SignInUserResult::GetTermsOfServiceUrlJson() const {
  return impl_->GetTermsOfServiceUrlJson();
}

const std::string& SignInUserResult::GetPrivatePolicyUrl() const {
  return impl_->GetPrivatePolicyUrl();
}

const std::string& SignInUserResult::GetPrivatePolicyUrlJson() const {
  return impl_->GetPrivatePolicyUrlJson();
}

SignInUserResult::SignInUserResult(std::shared_ptr<SignInUserResultImpl> impl)
    : SignInResult(impl), impl_(impl) {}

}  // namespace authentication
}  // namespace olp
