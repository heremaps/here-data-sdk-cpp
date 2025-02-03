/*
 * Copyright (C) 2023-2025 HERE Europe B.V.
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

#include <boost/optional/optional.hpp>

#include <olp/core/CoreApi.h>
#include <olp/core/http/CertificateSettings.h>

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
   * @brief Enable the underlying implementation to produce additional logs.
   * Note: This is EXPERIMENTAL settings, only CURL implementation supports it.
   */
  boost::optional<std::string> log_file_path;
};

}  // namespace http
}  // namespace olp
