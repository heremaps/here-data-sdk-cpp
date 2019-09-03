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

#include <cstdint>
#include <memory>
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/model/Data.h>

namespace olp {
namespace client {

class OlpClient;

}  // namespace client
}  // namespace olp

namespace olp {
namespace dataservice {
namespace read {

using DataResult = model::Data;
using DataResponse = client::ApiResponse<DataResult, client::ApiError>;
using DataResponseCallback = std::function<void(DataResponse response)>;

class DATASERVICE_READ_API VersionedLayerClient final {
 public:
  VersionedLayerClient(
      std::shared_ptr<olp::client::OlpClientSettings> client_settings,
      std::string hrn, std::string layer_id, std::int64_t layer_version);

  olp::client::CancellationToken GetDataByPartitionId(
      std::string partition_id, DataResponseCallback callback);

 private:
  std::shared_ptr<olp::client::OlpClient> olp_client_;
  std::shared_ptr<olp::client::OlpClientSettings> client_settings_;
  std::string hrn_;
  std::string layer_id_;
  std::int64_t layer_version_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
