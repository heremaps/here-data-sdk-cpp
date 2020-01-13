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

#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
class StreamLayerClientImpl;

/**
 * @brief Client that provides the ability to consume data from a stream layer
 * in real time. The client reads the data in the order it is added to the
 * queue. Once the client reads the data, the data is no longer available to
 * that client, but the data remains available to other clients.
 *
 * Example with creating the client:
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
 * @endcode
 *
 * @see
 * https://developer.here.com/olp/documentation/data-api/data_dev_guide/rest/layers/layers.html
 * @see
 * https://developer.here.com/olp/documentation/data-api/data_dev_guide/rest/getting-data-stream.html
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

 private:
  std::unique_ptr<StreamLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
