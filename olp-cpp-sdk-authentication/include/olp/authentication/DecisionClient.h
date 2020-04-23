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

#include <memory>

#include <olp/authentication/AuthorizeRequest.h>
#include <olp/authentication/Types.h>
#include <olp/core/client/OlpClientSettings.h>

namespace olp {

namespace authentication {

class DecisionClientImpl;

class AUTHENTICATION_API DecisionClient final {
 public:
  DecisionClient(client::OlpClientSettings settings);

  // Movable, non-copyable
  DecisionClient(const DecisionClient& other) = delete;
  DecisionClient(DecisionClient&& other) noexcept;
  DecisionClient& operator=(const DecisionClient& other) = delete;
  DecisionClient& operator=(DecisionClient&& other) noexcept;

  ~DecisionClient() = default;
  client::CancellationToken GetDecision(AuthorizeRequest request,
                                        AuthorizeCallback callback);
  client::CancellableFuture<AuthorizeResponse> GetDecision(
      AuthorizeRequest request);

 private:
  std::shared_ptr<DecisionClientImpl> impl_;
};

}  // namespace authentication
}  // namespace olp
