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

#include "olp/dataservice/read/VersionedLayerClient.h"

#include <olp/core/porting/make_unique.h>
#include "VersionedLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace read {

VersionedLayerClient::VersionedLayerClient(client::HRN catalog,
                                           std::string layer_id,
                                           client::OlpClientSettings settings)
    : impl_(std::make_unique<VersionedLayerClientImpl>(
          std::move(catalog), std::move(layer_id), boost::none,
          std::move(settings))) {}

VersionedLayerClient::VersionedLayerClient(
    client::HRN catalog, std::string layer_id,
    boost::optional<int64_t> catalog_version,
    client::OlpClientSettings settings)
    : impl_(std::make_unique<VersionedLayerClientImpl>(
          std::move(catalog), std::move(layer_id), std::move(catalog_version),
          std::move(settings))) {}

VersionedLayerClient::VersionedLayerClient(
    VersionedLayerClient&& other) noexcept = default;

VersionedLayerClient& VersionedLayerClient::operator=(
    VersionedLayerClient&& other) noexcept = default;

VersionedLayerClient::~VersionedLayerClient() = default;

bool VersionedLayerClient::CancelPendingRequests() {
  return impl_->CancelPendingRequests();
}

client::CancellationToken VersionedLayerClient::GetData(
    DataRequest data_request, DataResponseCallback callback) {
  return impl_->GetData(std::move(data_request), std::move(callback));
}

client::CancellableFuture<DataResponse> VersionedLayerClient::GetData(
    DataRequest data_request) {
  return impl_->GetData(std::move(data_request));
}

client::CancellationToken VersionedLayerClient::GetPartitions(
    PartitionsRequest partitions_request, PartitionsResponseCallback callback) {
  return impl_->GetPartitions(std::move(partitions_request),
                              std::move(callback));
}

client::CancellableFuture<PartitionsResponse>
VersionedLayerClient::GetPartitions(PartitionsRequest partitions_request) {
  return impl_->GetPartitions(std::move(partitions_request));
}

client::CancellationToken VersionedLayerClient::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback) {
  return impl_->PrefetchTiles(std::move(request), std::move(callback));
}

client::CancellableFuture<PrefetchTilesResponse>
VersionedLayerClient::PrefetchTiles(PrefetchTilesRequest request) {
  return impl_->PrefetchTiles(std::move(request));
}

client::CancellationToken VersionedLayerClient::GetData(
    TileRequest request, DataResponseCallback callback) {
  return impl_->GetData(std::move(request), std::move(callback));
}

client::CancellableFuture<DataResponse> VersionedLayerClient::GetData(
    TileRequest request) {
  return impl_->GetData(std::move(request));
}

bool VersionedLayerClient::RemoveFromCache(const std::string& partition_id) {
  return impl_->RemoveFromCache(partition_id);
}

bool VersionedLayerClient::RemoveFromCache(const geo::TileKey& tile) {
  return impl_->RemoveFromCache(tile);
}

client::CancellationToken VersionedLayerClient::GetAggregatedData(
    TileRequest request, AggregatedDataResponseCallback callback) {
  return impl_->GetAggregatedData(std::move(request), std::move(callback));
}

client::CancellableFuture<AggregatedDataResponse>
VersionedLayerClient::GetAggregatedData(TileRequest request) {
  return impl_->GetAggregatedData(std::move(request));
}

bool VersionedLayerClient::IsCached(const std::string& partition_id) const {
  return impl_->IsCached(partition_id);
}

bool VersionedLayerClient::IsCached(const geo::TileKey& tile) const {
  return impl_->IsCached(tile);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
