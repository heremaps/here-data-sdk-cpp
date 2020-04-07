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
#include <string>

#include <boost/optional.hpp>
#include <olp/core/http/NetworkProxySettings.h>
#include <olp/authentication/AuthenticationApi.h>

namespace olp {
namespace http {
class Network;
}

namespace thread {
class TaskScheduler;
}

namespace authentication {
/// The default token endpoint url.
static constexpr auto kHereAccountProductionUrl =
    "https://account.api.here.com";

/**
 * @brief Configures the `TokenEndpoint` instance.
 *
 * Contains settings that customize the `TokenEndpoint` class.
 */
struct AUTHENTICATION_API AuthenticationSettings {
  /**
   * @brief The configuration settings for the network layer.
   *
   * To remove any existing proxy settings, set to boost::none.
   */
  boost::optional<http::NetworkProxySettings> network_proxy_settings{};

  /**
   * @brief The network instance that is used to internally operate with the OLP
   * services.
   */
  std::shared_ptr<http::Network> network_request_handler{nullptr};

  /**
   * @brief The `TaskScheduler` class that is used to manage
   * the callbacks enqueue.
   *
   * If nullptr, all request calls are performed synchronously.
   */
  std::shared_ptr<thread::TaskScheduler> task_scheduler{nullptr};

  /**
   * @brief The server URL of the token endpoint.
   */
  std::string token_endpoint_url{kHereAccountProductionUrl};

  /**
   * @brief The maximum number of tokens that can be stored in LRU
   * in-memory cache.
   */
  size_t token_cache_limit{100u};

  /**
   * @brief Uses system system time in authentication requests rather than
   * requesting time from authentication server.
   */
  bool use_system_time{false};
};

}  // namespace authentication
}  // namespace olp
