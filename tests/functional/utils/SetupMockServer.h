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
#include "Client.h"

#include <string>
#include <vector>

#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <testutils/CustomParameters.hpp>
#include "MockServerHelper.h"
#include "generated/model/Api.h"

namespace mockserver {
class SetupMockServer {
 public:
  static olp::http::NetworkProxySettings CreateProxySettings() {
    const auto kMockServerHost =
        CustomParameters::getArgument("mock_server_host");
    const auto kMockServerPort =
        std::stoi(CustomParameters::getArgument("mock_server_port"));
    return olp::http::NetworkProxySettings()
        .WithHostname(kMockServerHost)
        .WithPort(kMockServerPort)
        .WithType(olp::http::NetworkProxySettings::Type::HTTP);
  }

  static std::shared_ptr<olp::client::OlpClientSettings> CreateSettings(
      std::shared_ptr<olp::http::Network> network) {
    const auto kAppId = "id";
    const auto kAppSecret = "secret";

    olp::authentication::Settings auth_settings({kAppId, kAppSecret});
    auth_settings.network_request_handler = network;

    // setup proxy
    auth_settings.network_proxy_settings =
        mockserver::SetupMockServer::CreateProxySettings();
    olp::authentication::TokenProviderDefault provider(auth_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    auto settings = std::make_shared<olp::client::OlpClientSettings>();
    settings->network_request_handler = network;
    settings->authentication_settings = auth_client_settings;
    settings->task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

    // setup proxy
    settings->proxy_settings =
        mockserver::SetupMockServer::CreateProxySettings();
    return settings;
  }

  static std::shared_ptr<mockserver::MockServerHelper> CreateMockServer(
      std::shared_ptr<olp::http::Network> network, std::string catalog) {
    // create client to set mock server expectations
    olp::client::OlpClientSettings olp_client_settings;
    olp_client_settings.network_request_handler = network;
    return std::make_shared<mockserver::MockServerHelper>(olp_client_settings,
                                                          catalog);
  }
};

}  // namespace mockserver
