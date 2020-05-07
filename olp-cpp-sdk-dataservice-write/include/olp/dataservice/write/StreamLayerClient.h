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
#include <vector>

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
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

/**
 * @brief Creates an instance of the default cache with the provided settings.
 * @param settings Cache settings.
 * @return DefaultCache instance.
 * @deprecated Use olp::client::OlpClientSettingsFactory::CreateDefaultCache()
 * instead. Will be remove by 06.2020.
 */
OLP_SDK_DEPRECATED(
    "Please use OlpClientSettingsFactory::CreateDefaultCache() instead. "
    "Will be removed by 06.2020")
std::shared_ptr<cache::KeyValueCache> CreateDefaultCache(
    cache::CacheSettings settings = {});

using PublishDataResult = olp::dataservice::write::model::ResponseOkSingle;
using PublishDataResponse =
    client::ApiResponse<PublishDataResult, client::ApiError>;
using PublishDataCallback = std::function<void(PublishDataResponse response)>;

using PublishSdiiResult = olp::dataservice::write::model::ResponseOk;
using PublishSdiiResponse =
    client::ApiResponse<PublishSdiiResult, client::ApiError>;
using PublishSdiiCallback = std::function<void(PublishSdiiResponse response)>;

/// @brief Client responsible for writing data to an OLP stream layer.
class DATASERVICE_WRITE_API StreamLayerClient {
 public:
  using FlushResponse = std::vector<PublishDataResponse>;
  using FlushCallback = std::function<void(FlushResponse response)>;

  /**
   * @brief StreamLayerClient Constructor.
   * @param catalog OLP HRN that specifies the catalog to which this client
   * writes.
   * @param client_settings \c StreamLayerClient settings used to control
   * the behaviour of flush mechanism and other StreamLayerClient specific
   * properties.
   * @param settings Client settings used to control behaviour of this
   * StreamLayerClient instance.
   */
  StreamLayerClient(client::HRN catalog,
                    StreamLayerClientSettings client_settings,
                    client::OlpClientSettings settings);

  /**
   * @brief Cancels all the ongoing publish operations that this client started.
   *
   * Returns instantly and does not wait for the callbacks.
   * Use this operation to cancel all the pending publish requests without
   * destroying the actual client instance.
   * @note This operation does not cancel publish requests queued by the \ref
   * Queue method.
   */
  void CancelPendingRequests();

  /**
   * @brief Publishes data to an OLP stream layer.
   * @note Content-Type for this request is implicitly based on the
   * layer metadata for the target layer on OLP.
   * @param request PublishDataRequest object that represents the parameters for
   * this PublishData call.
   * @return CancellableFuture that contains the PublishDataResponse.
   */
  olp::client::CancellableFuture<PublishDataResponse> PublishData(
      model::PublishDataRequest request);

  /**
   * @brief Publishes data to an OLP stream layer.
   * @note Content-Type for this request is set implicitly based on the
   * layer metadata for the target layer on OLP.
   * @param request PublishDataRequest object that represents the parameters for
   * this PublishData call.
   * @param callback PublishDataCallback that is called with the
   * PublishDataResponse when the operation completes.
   * @return CancellationToken that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishData(model::PublishDataRequest request,
                                             PublishDataCallback callback);

  /**
   * @brief Enqueues a PublishDataRequest that is sent over the wire.
   * @param request PublishDataRequest object that represents the parameters for
   * the call.
   * @return Optional boost that is boost::none if the queue call is
   * successful. Otherwise, it contains a string with error details.
   */
  boost::optional<std::string> Queue(model::PublishDataRequest request);

  /**
   * @brief Flush PublishDataRequests that were queued via the Queue
   * API.
   * @param request \c FlushRequest object thatr represents the parameters for
   * this \c Flush method call.
   * @return CancellableFuture that contains the FlushResponse.
   */
  olp::client::CancellableFuture<FlushResponse> Flush(
      model::FlushRequest request);

  /**
   * @brief Flush PublishDataRequests that were queued via the Queue
   * API.
   * @param request \c FlushRequest object that represents the parameters for
   * this \c Flush method call.
   * @param callback The callback that is called when all the flush
   * results (see \c FlushResponse) are available.
   * @return CancellationToken that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken Flush(model::FlushRequest request,
                                       FlushCallback callback);

  /**
   * @brief Sends a list of SDII messages to an OLP stream layer.
   * The SDII message data must be in the SDII MessageList protobuf format. For
   * more information, please see the OLP Sensor Data Ingestion Interface
   * documentation and schemas.
   * @param request PublishSdiiRequest object that represents the parameters for
   * this PublishSdii call.
   * @return CancellableFuture that contains the PublishSdiiResponse.
   */
  olp::client::CancellableFuture<PublishSdiiResponse> PublishSdii(
      model::PublishSdiiRequest request);

  /**
   * @brief Sends a list of SDII messages to an OLP stream layer.
   * The SDII message data must be in the SDII MessageList protobuf format. For
   * more information, please see the OLP Sensor Data Ingestion Interface
   * documentation and schemas.
   * @param request PublishSdiiRequest object that represents the parameters for
   * this PublishSdii call.
   * @param callback PublishSdiiCallback that is called with the
   * PublishSdiiResponse when the operation completes.
   * @return CancellationToken that can be used to cancel the ongoing
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
