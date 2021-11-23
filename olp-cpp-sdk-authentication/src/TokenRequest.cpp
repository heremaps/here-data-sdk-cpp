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

#include "TokenRequest.h"

namespace olp {
namespace authentication {

class TokenRequest::TokenRequestImpl {
 public:
  TokenRequestImpl(const std::chrono::seconds& expires_in)
      : expires_in_(expires_in) {}

  const std::chrono::seconds& GetExpiresIn() const { return expires_in_; }

 private:
  std::chrono::seconds expires_in_;
};

TokenRequest::TokenRequest(const std::chrono::seconds& expires_in)
    : impl_(std::make_shared<TokenRequest::TokenRequestImpl>(expires_in)) {}

std::chrono::seconds TokenRequest::GetExpiresIn() const {
  return impl_->GetExpiresIn();
}

}  // namespace authentication
}  // namespace olp
