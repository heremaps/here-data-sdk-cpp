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

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/porting/deprecated.h>
#include <olp/dataservice/write/DataServiceWriteApi.h>
#include <olp/dataservice/write/generated/model/ResponseOkSingle.h>
#include <olp/dataservice/write/model/DeleteIndexDataRequest.h>
#include <olp/dataservice/write/model/PublishIndexRequest.h>
#include <olp/dataservice/write/model/UpdateIndexRequest.h>

namespace olp {
namespace client {
struct Error;
class HRN;
}  // namespace client

namespace dataservice {
namespace write {
class IndexLayerClientImpl;

using PublishIndexResult = olp::dataservice::write::model::ResponseOkSingle;
using PublishIndexResponse =
    client::ApiResponse<PublishIndexResult, client::ApiError>;
using PublishIndexCallback = std::function<void(PublishIndexResponse response)>;

using DeleteIndexDataResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using DeleteIndexDataCallback =
    std::function<void(DeleteIndexDataResponse response)>;

using UpdateIndexResponse =
    client::ApiResponse<client::ApiNoResult, client::ApiError>;
using UpdateIndexCallback = std::function<void(UpdateIndexResponse response)>;

/**
 * @brief Client that is responsible for writing data to
 * a HERE platform index layer.
 *
 */
class DATASERVICE_WRITE_API IndexLayerClient {
 public:
  /**
   * @brief Creates the `IndexLayerClient` instance.
   * @param catalog The HRN that specifies the catalog to which this client
   * writes.
   * @param settings Client settings used to control the behavior of the client
   * instance.
   */
  IndexLayerClient(client::HRN catalog, client::OlpClientSettings settings);

  /**
   * @brief Cancels all the ongoing operations that this client started.
   *
   * Returns instantly and does not wait for the callbacks.
   * Use this operation to cancel all the pending requests without
   * destroying the actual client instance.
   */
  void CancelPendingRequests();

  /**
   * @brief Publishes an index to an index layer.
   * @note Content-Type for this request is set implicitly based on the
   * layer metadata for the target layer on the HERE platform.
   * @param request PublishIndexRequest object that represents the
   * parameters for this PublishIndex call.
   * @return CancellableFuture that contains the PublishIndexResponse.
   */
  olp::client::CancellableFuture<PublishIndexResponse> PublishIndex(
      model::PublishIndexRequest request);

  /**
   * @brief Publishes an index to an index layer.
   * @note Content-Type for this request is set implicitly based on the
   * layer metadata for the target layer on the HERE platform.
   * @param request PublishIndexRequest object that represents the
   * parameters for this PublishIndex call.
   * @param callback PublishIndexCallback that is called with the
   * PublishIndexResponse when the operation completes.
   * @return CancellationToken that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken PublishIndex(
      model::PublishIndexRequest request, PublishIndexCallback callback);

  /**
   * @brief Deletes a data blob that is stored under an index
   * layer.
   * @param request DeleteIndexDataRequest object that represents the parameters
   * for the DeleteIndexData call.
   * @param callback DeleteIndexDataCallback that is called with
   * DeleteIndexDataResponse when the operation completes.
   * @return CancellationToken that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken DeleteIndexData(
      model::DeleteIndexDataRequest request, DeleteIndexDataCallback callback);

  /**
   * @brief Deletes a data blob that is stored under an index
   * layer.
   * @param request DeleteIndexDataRequest object that represents the parameters
   * for DeleteIndexData call.
   * @return CancellableFuture that contains the DeleteIndexDataResponse.
   */
  olp::client::CancellableFuture<DeleteIndexDataResponse> DeleteIndexData(
      model::DeleteIndexDataRequest request);

  /**
   * @brief Updates index information to an index layer.
   * @param request UpdateIndexRequest object that represents the
   * parameters for this UpdateIndex call.
   * @return CancellableFuture that contains the PublishIndexResponse.
   */
  olp::client::CancellableFuture<UpdateIndexResponse> UpdateIndex(
      model::UpdateIndexRequest request);

  /**
   * @brief Updates index information to an index layer
   * @param request PublishIndexRequest object that represents the
   * parameters for this PublishIndex call.
   * @param callback PublishIndexCallback that is called with the
   * PublishIndexResponse when the operation completes.
   * @return CancellationToken that can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken UpdateIndex(model::UpdateIndexRequest request,
                                             UpdateIndexCallback callback);

 private:
  std::shared_ptr<IndexLayerClientImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
