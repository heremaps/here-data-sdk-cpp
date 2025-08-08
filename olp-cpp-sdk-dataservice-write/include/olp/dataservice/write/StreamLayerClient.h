/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
#include <vector>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/porting/deprecated.h>
#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/StreamLayerClientSettings.h>
#include <olp/dataservice/write/generated/model/ResponseOk.h>
#include <olp/dataservice/write/generated/model/ResponseOkSingle.h>
#include <olp/dataservice/write/model/FlushRequest.h>
#include <olp/dataservice/write/model/PublishDataRequest.h>
#include <olp/dataservice/write/model/PublishSdiiRequest.h>

namespace olp {
namespace client {
struct Error;
class HRN;
}  // namespace client

namespace dataservice {
namespace write {
class StreamLayerClientImpl;

using PublishDataResult = olp::dataservice::write::model::ResponseOkSingle;
using PublishDataResponse =
    client::ApiResponse<PublishDataResult, client::ApiError>;
using PublishDataCallback = std::function<void(PublishDataResponse response)>;

using PublishSdiiResult = olp::dataservice::write::model::ResponseOk;
using PublishSdiiResponse =
    client::ApiResponse<PublishSdiiResult, client::ApiError>;
using PublishSdiiCallback = std::function<void(PublishSdiiResponse response)>;

/// Publishes data to a stream layer.
class DATASERVICE_WRITE_API StreamLayerClient {
 public:
  /// An alias for the flush response.
  using FlushResponse = std::vector<PublishDataResponse>;

  /// An alias for the flush callback.
  using FlushCallback = std::function<void(FlushResponse response)>;

  /**
   * @brief Creates the `StreamLayerClient` insatnce.
   *
   * @param catalog The HRN of the catalog to which this client writes.
   * @param client_settings The `StreamLayerClient` settings used to control
   * the behavior of the flush mechanism and other `StreamLayerClient`
   * properties.
   * @param settings The client settings used to control the behavior
   * of the client instance.
   */
  StreamLayerClient(client::HRN catalog,
                    StreamLayerClientSettings client_settings,
                    client::OlpClientSettings settings);

  /**
   * @brief Cancels all the ongoing publish operations that this client started.
   *
   * Returns instantly and does not wait for callbacks.
   * Use this operation to cancel all the pending publish requests without
   * destroying the actual client instance.
   *
   * @note This operation does not cancel publish requests queued by the
   * `Queue` method.
   */
  void CancelPendingRequests();

  /**
   * @brief Publishes data to the stream layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param request The `PublishDataRequest` object.
   *
   * @return `CancellableFuture` that contains `PublishDataResponse`.
   */
  olp::client::CancellableFuture<PublishDataResponse> PublishData(
      model::PublishDataRequest request);

  /**
   * @brief Publishes data to the stream layer.
   *
   * @note The content-type for this request is set implicitly based on
   * the layer metadata of the target layer.
   *
   * @param request The `PublishDataRequest` object.
   * @param callback `PublishDataCallback` that is called with
   * `PublishDataResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishData(model::PublishDataRequest request,
                                             PublishDataCallback callback);

  /**
   * @brief Enqueues `PublishDataRequest` that is sent over the wire.
   *
   * @param request The `PublishDataRequest` object.
   *
   * @return An optional boost that is `olp::porting::none` if the queue call is
   * successful. Otherwise, it contains a string with error details.
   */
  porting::optional<std::string> Queue(model::PublishDataRequest request);

  /**
   * @brief Flushes `PublishDataRequests` that are queued via the Queue API.
   *
   * @param request The `FlushRequest` object.
   *
   * @return `CancellableFuture` that contains `FlushResponse`.
   */
  olp::client::CancellableFuture<FlushResponse> Flush(
      model::FlushRequest request);

  /**
   * @brief Flushes `PublishDataRequests` that are queued via the Queue API.
   *
   * @param request The `FlushRequest` object.
   * @param callback The callback that is called when all the flush
   * results (see `FlushResponse`) are available.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken Flush(model::FlushRequest request,
                                       FlushCallback callback);

  /**
   * @brief Sends a list of SDII messages to a stream layer.
   *
   * SDII message data must be in the SDII Message List protobuf format.
   * The maximum size is 20 MB.
   * For more information, see the HERE platform Sensor Data Ingestion Interface
   * documentation and schemas.
   *
   * @param request The `PublishSdiiRequest` object.
   *
   * @return `CancellableFuture` that contains `PublishSdiiResponse`.
   */
  olp::client::CancellableFuture<PublishSdiiResponse> PublishSdii(
      model::PublishSdiiRequest request);

  /**
   * @brief Sends a list of SDII messages to a stream layer.
   *
   * SDII message data must be in the SDII Message List protobuf format.
   * The maximum size is 20 MB.
   * For more information, see the HERE platform Sensor Data Ingestion Interface
   * documentation and schemas.
   *
   * @param request `PublishSdiiRequest` object.
   * @param callback `PublishSdiiCallback` that is called with
   * `PublishSdiiResponse` when the operation completes.
   *
   * @return `CancellationToken` that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishSdii(model::PublishSdiiRequest request,
                                             PublishSdiiCallback callback);

 private:
  std::shared_ptr<StreamLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
