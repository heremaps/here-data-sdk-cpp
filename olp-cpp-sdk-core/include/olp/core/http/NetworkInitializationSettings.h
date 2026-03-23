/*
 * Copyright (C) 2023-2026 HERE Europe B.V.
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

#include <string>

#include <olp/core/CoreApi.h>
#include <olp/core/http/CertificateSettings.h>
#include <olp/core/porting/optional.h>

namespace olp {
namespace http {

/**
 * @brief Settings for network initialization.
 */
struct CORE_API NetworkInitializationSettings {
  /**
   * @brief The maximum number of requests that can be sent simultaneously.
   */
  size_t max_requests_count = 30u;

  /**
   * @brief The custom certificate settings.
   */
  CertificateSettings certificate_settings;

  /**
   * @brief The path to the network diagnostic output.
   * If not set, diagnostics will not be produced.
   *
   * @note Currently, only CURL-based network implementation supports this
   * setting.
   */
  porting::optional<std::string> diagnostic_output_path = porting::none;

  /**
   * @brief Global transfer limit in bytes per second. A value of 0 means no
   * limit. This value is applied per connection to cURL via
   * CURLOPT_MAX_RECV_SPEED_LARGE / CURLOPT_MAX_SEND_SPEED_LARGE where
   * supported.
   */
  size_t max_transfer_bytes_per_second = 0u;
};

}  // namespace http
}  // namespace olp
