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

#include "VolatileLayerClientImpl.h"

#include <olp/core/client/CancellationContext.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsRepository.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "VolatileLayerClientImpl";
}

VolatileLayerClientImpl::VolatileLayerClientImpl(
    olp::client::HRN catalog, std::string layer_id,
    olp::client::OlpClientSettings client_settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      client_settings_(std::move(client_settings)) {}

VolatileLayerClientImpl::~VolatileLayerClientImpl() = default;

olp::client::CancellationToken VolatileLayerClientImpl::GetData(
    DataRequest data_request, Callback callback) {
  olp::client::CancellationContext context;
  auto cancellation_token = olp::client::CancellationToken(
      [context]() mutable { context.CancelOperation(); });

  auto catalog = catalog_;
  auto client_settings = client_settings_;
  auto layer_id = layer_id_;

  repository::ExecuteOrSchedule(&client_settings_, [=]() mutable {
    if (!data_request.GetDataHandle()) {
      auto response = repository::PartitionsRepository::GetPartitionById(
          catalog, layer_id, context, data_request, client_settings);

      if (!response.IsSuccessful()) {
        callback(response.GetError());
        return;
      }

      const auto& partitions = response.GetResult().GetPartitions();
      if (partitions.empty()) {
        OLP_SDK_LOG_INFO_F(kLogTag, "Partition %s not found",
                           data_request.GetPartitionId());
        callback(client::ApiError(client::ErrorCode::NotFound,
                                  "Partition not found"));
        return;
      }

      data_request.WithDataHandle(partitions.front().GetDataHandle());
    }

    auto blob_response = repository::DataRepository::GetBlobData(
        catalog_, layer_id_, "volatile-blob", data_request, context,
        client_settings_);

    callback(blob_response);
  });

  return cancellation_token;
}  // namespace read

}  // namespace read
}  // namespace dataservice
}  // namespace olp
