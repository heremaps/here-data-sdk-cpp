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

#include "olp/core/client/OlpClientFactory.h"

#include "olp/core/client/OlpClient.h"
#include "olp/core/client/OlpClientSettings.h"

namespace olp {
namespace client {

std::shared_ptr<OlpClient> OlpClientFactory::Create(
    const OlpClientSettings& settings) {
  auto olp_client = std::make_shared<OlpClient>();
  olp_client->SetSettings(settings);
  return olp_client;
}

}  // namespace client
}  // namespace olp
