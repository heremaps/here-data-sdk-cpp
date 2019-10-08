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

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/model/Data.h>

namespace olp {
namespace dataservice {
namespace read {
class VolatileLayerClientImpl;

/**
 * @brief The VolatileLayerClient aimed to acquire data from volatile layer of
 * OLP. A volatile layer is a key/value store where values for a given key can
 * change and only the latest value is retrievable.
 *
 * Example:
 * @code{.cpp}
 * const std::string kCatalog = "hrn:here:data:::hereos-internal-test-v2";
 * const std::string kLayerId = "hype-test";
 * const auto kHRN = olp::client::HRN::FromString(kCatalog);
 *
 * std::shared_ptr<olp::thread::TaskScheduler> task_scheduler =
 *    olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);
 * std::shared_ptr<olp::http::Network> http_client = olp::client::
 *    OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
 *
 * olp::authentication::Settings settings;
 * settings.task_scheduler = task_scheduler;
 * settings.network_request_handler = http_client;
 *
 * olp::client::AuthenticationSettings auth_settings;
 * auth_settings.provider = olp::authentication::TokenProviderDefault(
 *     kKeyId, kKeySecret, std::move(settings));
 *
 * auto client_settings = olp::client::OlpClientSettings();
 * client_settings.authentication_settings = auth_settings;
 * client_settings.task_scheduler = std::move(task_scheduler);
 * client_settings.network_request_handler = std::move(http_client);
 *
 * VolatileLayerClient client{kHRN, kLayerId, client_settings};
 * VolatileLayerClient::Callback<model::Data> callback = ... ;
 * auto token = client.GetData(request, cb);
 * @endcode
 *
 * @see
 * https://developer.here.com/olp/documentation/data-api/data_dev_guide/shared_content/topics/olp/concepts/layers.html
 */
class DATASERVICE_READ_API VolatileLayerClient final {
 public:
  /// DataResult alias
  using DataResult = model::Data;
  /// ApiResponse template alias
  template <typename Result>
  using CallbackResponse = client::ApiResponse<Result, client::ApiError>;
  /// Callback template alias
  template <typename Response>
  using Callback = std::function<void(CallbackResponse<Response> response)>;

  /**
   * @brief VolatileLayerClient constructor
   * @param catalog this volatile layer client uses during requests.
   * @param layer_id this volatile layer client uses during requests.
   * @param settings the settings used to control behaviour of the client
   * instance.
   */
  VolatileLayerClient(olp::client::HRN catalog, std::string layer_id,
                      olp::client::OlpClientSettings client_settings);

  ~VolatileLayerClient();

  /**
   * @brief Fetches data for a partition or data handle asynchronously. If the
   * specified partition or data handle cannot be found in the layer, the
   * callback will be invoked with an empty DataResponse (nullptr for result and
   * error). If neither Partition Id or Data Handle were set in the request, the
   * callback will be invoked with an error with ErrorCode::InvalidRequest.
   * @param data_request contains the complete set of request parameters.
   * @param callback will be invoked once the DataResult is available, or an
   * error is encountered.
   * @return A token that can be used to cancel this request.
   */
  olp::client::CancellationToken GetData(DataRequest data_request,
                                         Callback<DataResult> callback);

 private:
  std::unique_ptr<VolatileLayerClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
