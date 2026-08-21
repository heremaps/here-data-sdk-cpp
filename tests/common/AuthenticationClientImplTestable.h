/*
 * Copyright (C) 2026 HERE Europe B.V.
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
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include "AuthenticationClientImpl.h"

namespace mocks {

class AuthenticationClientImplTestable
    : public olp::authentication::AuthenticationClientImpl {
 public:
  explicit AuthenticationClientImplTestable(
      olp::authentication::AuthenticationSettings settings)
      : AuthenticationClientImpl(settings) {}

  MOCK_METHOD(olp::authentication::TimeResponse, GetTimeFromServer,
              (olp::client::CancellationContext context,
               const olp::client::OlpClient& client),
              (const, override));

  MOCK_METHOD(olp::client::HttpResponse, CallAuth,
              (const olp::client::OlpClient&, const std::string&,
               olp::client::CancellationContext,
               const olp::authentication::AuthenticationCredentials&,
               olp::client::OlpClient::RequestBodyType, std::time_t,
               const std::string&),
              (override));

  MOCK_METHOD(std::shared_ptr<olp::http::Network>, CreateNetworkRequestHandler,
              (olp::http::NetworkInitializationSettings settings),
              (const, override));

  olp::client::HttpResponse RealCallAuth(
      const olp::client::OlpClient& client, const std::string& endpoint,
      olp::client::CancellationContext context,
      const olp::authentication::AuthenticationCredentials& credentials,
      olp::client::OlpClient::RequestBodyType body, std::time_t time,
      const std::string& content_type) {
    return olp::authentication::AuthenticationClientImpl::CallAuth(
        client, endpoint, std::move(context), credentials, std::move(body),
        time, content_type);
  }
};

}  // namespace mocks
