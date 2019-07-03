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
 * @brief client namespace
 */
namespace client {
/**
 * @brief Factory that creates OlpClient instances that is used for all REST
 * requests.
 */

class CORE_API OlpClientFactory {
 public:
  /**
   * @brief Creates an OlpClient instance used for all REST requests.
   * @param settings Settings for the OlpClient.
   *
   * @return An instance of OlpClient.
   */
  static std::shared_ptr<OlpClient> Create(const OlpClientSettings& settings);
};

}  // namespace client

}  // namespace olp
