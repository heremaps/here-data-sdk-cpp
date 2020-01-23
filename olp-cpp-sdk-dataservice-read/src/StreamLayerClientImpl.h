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

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/dataservice/read/SubscribeRequest.h>
#include <olp/dataservice/read/Types.h>

namespace olp {
namespace dataservice {
namespace read {

class StreamLayerClientImpl {
 public:
  StreamLayerClientImpl(client::HRN catalog, std::string layer_id,
                        client::OlpClientSettings settings);

  virtual ~StreamLayerClientImpl();

  virtual bool CancelPendingRequests();

  virtual client::CancellationToken Subscribe(
      SubscribeRequest request, SubscribeResponseCallback callback);

  virtual client::CancellableFuture<SubscribeResponse> Subscribe(
      SubscribeRequest request);

  virtual client::CancellationToken Unsubscribe(
      UnsubscribeResponseCallback callback);

  virtual client::CancellableFuture<UnsubscribeResponse> Unsubscribe();

 private:
  /// A struct that aggregates the stream layer client parameters.
  struct StreamLayerClientContext {
    StreamLayerClientContext(std::string subscription_id,
                             std::string subscription_mode,
                             std::string node_base_url,
                             std::string x_correlation_id)
        : subscription_id(subscription_id),
          subscription_mode(subscription_mode),
          node_base_url(node_base_url),
          x_correlation_id(x_correlation_id) {}

    std::string subscription_id;
    std::string subscription_mode;
    std::string node_base_url;
    std::string x_correlation_id;
  };

  client::HRN catalog_;
  std::string layer_id_;
  client::OlpClientSettings settings_;
  std::shared_ptr<client::PendingRequests> pending_requests_;
  std::mutex mutex_;
  std::unique_ptr<StreamLayerClientContext> client_context_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
