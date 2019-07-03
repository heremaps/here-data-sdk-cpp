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

#include "olp/core/client/OlpClient.h"
#include "olp/core/client/OlpClientSettings.h"
#include "olp/core/network/Network.h"

namespace olp {
namespace client {
class NetworkAsyncHandlerImpl {
 public:
  CancellationToken operator()(const network::NetworkRequest& request,
                               const network::NetworkConfig& config,
                               const NetworkAsyncCallback& callback);

 protected:
  std::shared_ptr<network::Network> GetNetwork(
      const network::NetworkConfig& config);

 private:
  CancellationToken ExecuteSingleRequest(
      std::shared_ptr<network::Network> network,
      const network::NetworkRequest& request,
      const NetworkAsyncCallback& callback);
};

}  // namespace client
}  // namespace olp
