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

#pragma once

#include <boost/optional.hpp>
#include <memory>
#include <string>

#include <olp/core/http/NetworkProxySettings.h>

#include "AuthenticationApi.h"

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
 * @brief Settings which can be used to configure a TokenEndpoint instance.
 */
struct AUTHENTICATION_API Settings {
  /// Optional. network_proxy_settings containing proxy configuration settings
  /// for the network layer.
  boost::optional<http::NetworkProxySettings> network_proxy_settings;

  /// The network instance to be used to internally operate with OLP services.
  std::shared_ptr<http::Network> network_request_handler;

  /// The TaskScheduler class to be used for managing callbacks enqueue
  std::shared_ptr<thread::TaskScheduler> task_scheduler;

  /// The token endpoint server URL. Note that only standard OAuth2 Token URLs
  /// (those ending in 'oauth2/token') are supported.
  std::string token_endpoint_url{kHereAccountProductionTokenUrl};
};

}  // namespace authentication
}  // namespace olp
