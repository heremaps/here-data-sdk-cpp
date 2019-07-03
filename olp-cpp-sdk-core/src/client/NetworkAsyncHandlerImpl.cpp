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

#include "NetworkAsyncHandlerImpl.h"

#include <sstream>

#include <olp/core/network/HttpResponse.h>
#include "olp/core/network/NetworkConfig.h"
#include "olp/core/network/NetworkRequest.h"
#include "olp/core/network/NetworkResponse.h"

namespace olp {
namespace client {
CancellationToken NetworkAsyncHandlerImpl::operator()(
    const network::NetworkRequest& request,
    const network::NetworkConfig& config,
    const NetworkAsyncCallback& callback) {
  auto network = GetNetwork(config);

  return ExecuteSingleRequest(network, request, callback);
}

std::shared_ptr<network::Network> NetworkAsyncHandlerImpl::GetNetwork(
    const network::NetworkConfig& config) {
  // TODO Could this be a weak_ptr, allowing reuse if there's other requests
  // running, and resource release if not?
  auto network = std::make_shared<network::Network>();
  network->Start(config);
  return network;
}

CancellationToken NetworkAsyncHandlerImpl::ExecuteSingleRequest(
    std::shared_ptr<network::Network> network,
    const network::NetworkRequest& request,
    const NetworkAsyncCallback& callback) {
  auto requestId = network->Send(
      request, std::make_shared<std::stringstream>(),
      [network, callback](const network::NetworkResponse& response) {
        // status is -1 if it's timed-out
        network::HttpResponse result(response.Status());

        if (result.status >= 400 || result.status < 0) {
          result.response = "Error occured. Please check HTTP status code.";

          std::string payload_stream;
          if (response.PayloadStream()) {
            std::ostringstream errorMsgBuf;
            errorMsgBuf << response.PayloadStream()->rdbuf();
            payload_stream = errorMsgBuf.str();
          }

          if (!payload_stream.empty()) {
            result.response = std::move(payload_stream);
          } else if (!response.Error().empty()) {
            result.response = std::move(response.Error());
          }
        } else {
          std::ostringstream buf;
          buf << response.PayloadStream()->rdbuf();
          result.response = buf.str();
        }

        callback(result);
      });

  std::weak_ptr<network::Network> wkNetwork(network);

  return CancellationToken([wkNetwork, requestId]() {
    auto network = wkNetwork.lock();

    if (network) {
      network->Cancel(requestId);
    }
  });
}

}  // namespace client
}  // namespace olp
