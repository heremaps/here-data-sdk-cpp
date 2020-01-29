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

#include <memory>
#include <string>

#include "OlpClientSettings.h"

#include <olp/core/CoreApi.h>

namespace olp {
namespace client {
class OlpClient;
}

/**
 * @brief The `client` namespace.
 */
namespace client {
/**
 * @brief Creates the `OlpClient` instances that are used for every HTTP
 * request.
 */

class CORE_API OlpClientFactory {
 public:
  /**
   * @brief Creates the `OlpClient` instance used for every HTTP request.
   *
   * @param settings The `OlpClientSettings` instance.
   *
   * @return The OlpClient instance.
   */
  static std::shared_ptr<OlpClient> Create(const OlpClientSettings& settings);
};

}  // namespace client

}  // namespace olp
