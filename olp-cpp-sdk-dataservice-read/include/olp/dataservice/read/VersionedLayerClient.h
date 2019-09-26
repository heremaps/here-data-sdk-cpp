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
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/Data.h>

namespace olp {
namespace dataservice {
namespace read {

using DataResult = model::Data;
using DataResponse = client::ApiResponse<DataResult, client::ApiError>;
using DataResponseCallback = std::function<void(DataResponse response)>;

/**
 * @brief The VersionedLayerClient aimed to acquire data from OLP services.
 */
class DATASERVICE_READ_API VersionedLayerClient final {
 public:
  /**
   * @brief Sets up \c OlpClientSettings and \c HRN to be used during requests.
   */
  VersionedLayerClient(olp::client::OlpClientSettings client_settings,
                       olp::client::HRN hrn);

  ~VersionedLayerClient();

  /**
   * @brief Sends data from OLP server described in DataRequest to a callback.
   * @param data_request \c DataRequest class that defines which catalog,
   * version, layer and partition or data handle to be queried. Will contact
   * Blob service directly if data handle is set. Will query meta data service
   * otherwise.
   * @param callback to be called once data is ready or the request is
   * cancelled.
   * @return A CancellationToken which can be used to cancel the ongoing
   * request.
   */
  olp::client::CancellationToken GetData(DataRequest data_request,
                                         DataResponseCallback callback);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
