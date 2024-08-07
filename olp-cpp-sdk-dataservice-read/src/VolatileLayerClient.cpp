/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/VolatileLayerClient.h>

#include "VolatileLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace read {

VolatileLayerClient::VolatileLayerClient(client::HRN catalog,
                                         std::string layer_id,
                                         client::OlpClientSettings settings)
    : impl_(std::make_unique<VolatileLayerClientImpl>(
          std::move(catalog), std::move(layer_id), std::move(settings))) {}

VolatileLayerClient::VolatileLayerClient(VolatileLayerClient&& other) noexcept =
    default;

VolatileLayerClient& VolatileLayerClient::operator=(
    VolatileLayerClient&& other) noexcept = default;

VolatileLayerClient::~VolatileLayerClient() = default;

bool VolatileLayerClient::CancelPendingRequests() {
  return impl_->CancelPendingRequests();
}

client::CancellationToken VolatileLayerClient::GetPartitions(
    PartitionsRequest request, PartitionsResponseCallback callback) {
  return impl_->GetPartitions(std::move(request), std::move(callback));
}

olp::client::CancellableFuture<PartitionsResponse>
VolatileLayerClient::GetPartitions(PartitionsRequest request) {
  return impl_->GetPartitions(std::move(request));
}

client::CancellationToken VolatileLayerClient::GetData(
    DataRequest request, DataResponseCallback callback) {
  return impl_->GetData(std::move(request), std::move(callback));
}

olp::client::CancellableFuture<DataResponse> VolatileLayerClient::GetData(
    DataRequest request) {
  return impl_->GetData(std::move(request));
}

bool VolatileLayerClient::RemoveFromCache(const std::string& partition_id) {
  return impl_->RemoveFromCache(partition_id);
}

bool VolatileLayerClient::RemoveFromCache(const geo::TileKey& tile) {
  return impl_->RemoveFromCache(tile);
}

client::ApiNoResponse VolatileLayerClient::DeleteFromCache(
    const std::string& partition_id) {
  return impl_->DeleteFromCache(partition_id);
}

client::ApiNoResponse VolatileLayerClient::DeleteFromCache(
    const geo::TileKey& tile) {
  return impl_->DeleteFromCache(tile);
}

client::CancellationToken VolatileLayerClient::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback) {
  return impl_->PrefetchTiles(std::move(request), std::move(callback));
}

client::CancellableFuture<PrefetchTilesResponse>
VolatileLayerClient::PrefetchTiles(PrefetchTilesRequest request) {
  return impl_->PrefetchTiles(std::move(request));
}
}  // namespace read
}  // namespace dataservice
}  // namespace olp
