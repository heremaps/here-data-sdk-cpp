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
#include "NetworkTestBase.h"

#include <memory>

#include <gtest/gtest.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkSettings.h>

void NetworkTestBase::SetUp() {
  auto proxy_settings =
      olp::http::NetworkProxySettings()
          .WithHostname("localhost")
          .WithPort(1080)
          .WithUsername("test_user")
          .WithPassword("test_password")
          .WithType(olp::http::NetworkProxySettings::Type::HTTP);

  settings_ = olp::http::NetworkSettings().WithProxySettings(proxy_settings);
  network_ = olp::client::OlpClientSettingsFactory::
      CreateDefaultNetworkRequestHandler();

  olp::client::OlpClientSettings client_settings;
  client_settings.network_request_handler = network_;
  mock_server_client_ = std::make_shared<mockserver::Client>(client_settings);
  mock_server_client_->Reset();
}
