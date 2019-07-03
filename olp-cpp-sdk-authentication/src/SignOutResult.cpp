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

#include "olp/authentication/SignOutResult.h"

#include "SignOutResultImpl.h"

namespace olp {
namespace authentication {
SignOutResult::~SignOutResult() = default;

int SignOutResult::GetStatus() const { return impl_->GetStatus(); }

const ErrorResponse& SignOutResult::GetErrorResponse() const {
  return impl_->GetErrorResponse();
}

const ErrorFields& SignOutResult::GetErrorFields() const {
  return impl_->GetErrorFields();
}
SignOutResult::SignOutResult(std::shared_ptr<SignOutResultImpl> impl)
    : impl_(impl) {}

}  // namespace authentication
}  // namespace olp
