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

#include "PendingRequests.h"

#include "generated/api/BlobApi.h"
#include "generated/api/MetadataApi.h"
#include "generated/api/QueryApi.h"

#include "olp/core/client/ApiError.h"
#include "olp/core/client/ApiResponse.h"
#include "olp/core/client/CancellationContext.h"
#include "olp/core/client/HRN.h"
#include "olp/core/client/OlpClient.h"
#include "olp/dataservice/read/DataRequest.h"

namespace olp {
namespace dataservice {
namespace read {

class VersionedLayerClientImpl {
 public:
  /// DataResult alias
  using DataResult = model::Data;
  /// ApiResponse template alias
  template <typename Result>
  using CallbackResponse = client::ApiResponse<Result, client::ApiError>;
  /// Callback template alias
  template <typename Response>
  using Callback = std::function<void(CallbackResponse<Response>)>;

  VersionedLayerClientImpl(client::HRN catalog, std::string layer,
                           client::OlpClientSettings client_settings);

  ~VersionedLayerClientImpl();

  olp::client::CancellationToken GetData(DataRequest data_request,
                                         Callback<DataResult> callback);

 protected:
  virtual CallbackResponse<model::Data> GetData(
      client::CancellationContext cancellationContext,
      DataRequest request) const;

 private:
  const client::HRN catalog_;
  const std::string layer_;
  const client::OlpClientSettings settings_;
  /* a copy of settings with empty task scheduler, potentially could happen that
   * scheduled task owns the last point to task scheduler, this will result in a
   * deadlock */
  client::OlpClientSettings safe_settings_;
  std::shared_ptr<PendingRequests> pending_requests_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
