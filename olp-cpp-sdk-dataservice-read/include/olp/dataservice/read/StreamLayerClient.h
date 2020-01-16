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

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/SubscribeRequest.h>
#include <olp/dataservice/read/Types.h>

namespace olp {
namespace dataservice {
namespace read {
class StreamLayerClientImpl;

/**
 * @brief Provides the ability to consume data from a stream layer
 * in real time.
 *
 * The client reads the data in the order it is added to the queue.
 * Once the client reads the data, the data is no longer available to
 * that client, but the data remains available to other clients.
 *
 * Example of subscribing to a stream layer:
 * @code{.cpp}
 * auto task_scheduler =
 *    olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);
 * auto http_client = olp::client::
 *    OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
 *
 * olp::authentication::Settings settings{"Your.Key.Id", "Your.Key.Secret"};
 * settings.task_scheduler = task_scheduler;
 * settings.network_request_handler = http_client;
 *
 * olp::client::AuthenticationSettings auth_settings;
 * auth_settings.provider =
 *     olp::authentication::TokenProviderDefault(std::move(settings));
 *
 * auto client_settings = olp::client::OlpClientSettings();
 * client_settings.authentication_settings = auth_settings;
 * client_settings.task_scheduler = std::move(task_scheduler);
 * client_settings.network_request_handler = std::move(http_client);
 *
 * StreamLayerClient client{"hrn:here:data:::your-catalog-hrn", "your-layer-id",
 * client_settings};
 *
 * auto callback =
 *     [](olp::dataservice::read::SubscribeResponse){};
 * using SubscribeRequest = olp::dataservice::read::SubscribeRequest;
 *
 * auto request =
 * olp::dataservice::read::SubscribeRequest().WithSubscriptionMode(
 *             olp::dataservice::read::SubscribeRequest::SubscriptionMode::kSerial));
 * auto cancellable_future = stream_client.Subscribe(request);
 * @endcode
 *
 * @see The
 * [Layers](https://developer.here.com/olp/documentation/data-api/data_dev_guide/rest/layers/layers.html)
 * and [Get Data from a Stream
 * Layer](https://developer.here.com/olp/documentation/data-api/data_dev_guide/rest/getting-data-stream.html)
 * sections in the Data API Developer Guide.
 */
class DATASERVICE_READ_API StreamLayerClient final {
 public:
  /**
   * @brief StreamLayerClient constructor.
   * @param catalog the catalog's HRN that the stream layer client uses during
   * requests.
   * @param layer_id an ID of the layer that the client uses during
   * requests.
   * @param settings settings the client instance settings.
   */
  StreamLayerClient(client::HRN catalog, std::string layer_id,
                    client::OlpClientSettings settings);

  // Movable, non-copyable
  StreamLayerClient(const StreamLayerClient& other) = delete;
  StreamLayerClient(StreamLayerClient&& other) noexcept;
  StreamLayerClient& operator=(const StreamLayerClient& other) = delete;
  StreamLayerClient& operator=(StreamLayerClient&& other) noexcept;

  ~StreamLayerClient();

  /**
   * @brief Cancels all the active and pending requests.
   *
   * @return True on success.
   */
  bool CancelPendingRequests();

  /**
   * @brief Enables message consumption for the specific stream layer.
   *
   * @param request  The `SubscribeRequest` instance that contains a complete
   * set of request parameters.
   * @param callback The `SubscribeResponseCallback` object that is invoked when
   * the subscription request is completed.
   *
   * @return A token that can be used to cancel this request.
   */
  client::CancellationToken Subscribe(SubscribeRequest request,
                                      SubscribeResponseCallback callback);

  /**
   * @brief Enables message consumption for the specific stream layer.
   *
   * @param request The `SubscribeRequest` instance that contains a complete set
   * of request parameters.
   *
   * @return `CancellableFuture` that contains
   * `SubscribeId` or an error. You can also use `CancellableFuture` to cancel
   * this request.
   */
  client::CancellableFuture<SubscribeResponse> Subscribe(
      SubscribeRequest request);

 private:
  std::unique_ptr<StreamLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
