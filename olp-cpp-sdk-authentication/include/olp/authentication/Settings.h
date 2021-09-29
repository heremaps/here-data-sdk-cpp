/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <boost/optional.hpp>
#include <memory>
#include <string>

#include <olp/core/client/RetrySettings.h>
#include <olp/core/http/NetworkProxySettings.h>

#include "AuthenticationApi.h"
#include "AuthenticationCredentials.h"

namespace olp {
namespace http {
class Network;
}

namespace thread {
class TaskScheduler;
}

namespace authentication {
static const std::string kHereAccountProductionTokenUrl =
    "https://account.api.here.com/oauth2/token";

/**
 * @brief Configures the `TokenEndpoint` instance.
 *
 * Contains settings that customize the `TokenEndpoint` class.
 */
struct AUTHENTICATION_API Settings {
  /**
   * @brief Creates the `Settings` instance.
   *
   * @param credentials Your access credentials to the HERE platform.
   */
  Settings(AuthenticationCredentials credentials)
      : credentials(std::move(credentials)) {}

  /**
   * @brief The access key ID and access key secret that you got from the HERE
   * Account as a part of the onboarding or support process on the developer
   * portal.
   */
  AuthenticationCredentials credentials;

  /**
   * @brief The network instance that is used to internally operate with
   * the HERE platform Services.
   */
  std::shared_ptr<http::Network> network_request_handler;

  /**
   * @brief (Optional) The `TaskScheduler` class that is used to manage
   * the callbacks enqueue.
   */
  std::shared_ptr<thread::TaskScheduler> task_scheduler;

  /**
   * @brief (Optional) The configuration settings for the network layer.
   */
  boost::optional<http::NetworkProxySettings> network_proxy_settings;

  /**
   * @brief (Optional) The server URL of the token endpoint.
   *
   * @note Only standard OAuth2 Token URLs (those ending in `oauth2/token`) are
   * supported.
   */
  std::string token_endpoint_url{kHereAccountProductionTokenUrl};

  /**
   * @brief Uses system system time in authentication requests rather than
   * requesting time from authentication server.
   *
   * Default is true, which means system time is used.
   *
   * @note Please make sure that the system time does not deviate from the
   * official UTC time as it might result in error responses from the
   * authentication server.
   */
  bool use_system_time{true};

  /**
   * @brief A collection of settings that controls how failed requests should be
   * treated.
   */
  client::RetrySettings retry_settings;
};

}  // namespace authentication
}  // namespace olp
