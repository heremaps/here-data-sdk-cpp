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

#include <olp/authentication/AuthenticationApi.h>
#include <olp/authentication/MtlsProperties.h>
#include <olp/core/client/RetrySettings.h>
#include <olp/core/http/NetworkProxySettings.h>
#include <olp/core/porting/optional.h>

namespace olp {
namespace thread {
class TaskScheduler;
}

namespace authentication {

/// The default mTLS token endpoint URL.
static constexpr auto kHereMtlsAccountProductionUrl =
    "https://mtls.account.api.here.com/mtls/token";

/**
 * @brief Configures the `MtlsTokenProvider` instance.
 */
struct AUTHENTICATION_API MtlsSettings {
  /**
   * @brief The client certificate, key, scope, and expiration settings used
   * for the mTLS token request.
   */
  MtlsProperties mtls_properties;

  /**
   * @brief (Optional) The configuration settings for the network layer.
   */
  porting::optional<http::NetworkProxySettings> network_proxy_settings;

  /**
   * @brief (Optional) The mTLS token endpoint URL.
   */
  std::string token_endpoint_url{kHereMtlsAccountProductionUrl};

  /**
   * @brief A collection of settings that controls how failed requests should be
   * treated.
   */
  client::RetrySettings retry_settings;
};

}  // namespace authentication
}  // namespace olp
