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

#include <PendingRequests.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/model/Data.h>

namespace olp {
namespace dataservice {
namespace read {

class VersionedLayerClientImpl {
 public:
  /// DataResult alias
  using DataResult = model::Data;
  /// CallbackResponse alias
  using CallbackResponse = client::ApiResponse<DataResult, client::ApiError>;
  /// Callback alias
  using Callback = std::function<void(CallbackResponse response)>;

  VersionedLayerClientImpl(olp::client::HRN catalog, std::string layer_id,
                           olp::client::OlpClientSettings client_settings);

  virtual ~VersionedLayerClientImpl();

  virtual olp::client::CancellationToken GetData(DataRequest data_request,
                                                 Callback callback) const;

  static CallbackResponse GetDataSync(
      olp::client::HRN catalog, std::string layer_id,
      olp::client::OlpClientSettings client_settings, DataRequest data_request,
      olp::client::CancellationContext context);

 private:
  olp::client::CancellationToken AddGetDataTask(DataRequest data_request,
                                                Callback callback) const;

 protected:
  olp::client::HRN catalog_;
  std::string layer_id_;
  olp::client::OlpClientSettings sync_settings_;
  olp::client::OlpClientSettings scheduler_only_settings_;
  std::shared_ptr<PendingRequests> pending_requests_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
