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

#include "olp/dataservice/read/VersionedLayerClient.h"

#include <olp/core/porting/make_unique.h>
#include "VersionedLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace read {

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

client::CancellationToken VersionedLayerClient::StreamLayerPartitions(
    PartitionsRequest partitions_request,
    PartitionsStreamCallback partition_stream_callback,
    CallbackNoResult callback) {
  return impl_->StreamLayerPartitions(std::move(partitions_request),
                                      std::move(partition_stream_callback),
                                      std::move(callback));
}

client::CancellableFuture<PartitionsResponse>
VersionedLayerClient::GetPartitions(PartitionsRequest partitions_request) {
  return impl_->GetPartitions(std::move(partitions_request));
}

client::CancellationToken VersionedLayerClient::QuadTreeIndex(
    TileRequest tile_request, PartitionsResponseCallback callback) {
  return impl_->QuadTreeIndex(std::move(tile_request), std::move(callback));
}

client::CancellationToken VersionedLayerClient::PrefetchTiles(
    PrefetchTilesRequest request, PrefetchTilesResponseCallback callback,
    PrefetchStatusCallback status_callback) {
  return impl_->PrefetchTiles(std::move(request), std::move(callback),
                              std::move(status_callback));
}

client::CancellableFuture<PrefetchTilesResponse>
VersionedLayerClient::PrefetchTiles(PrefetchTilesRequest request,
                                    PrefetchStatusCallback status_callback) {
  return impl_->PrefetchTiles(std::move(request), std::move(status_callback));
}

client::CancellationToken VersionedLayerClient::PrefetchPartitions(
    PrefetchPartitionsRequest request,
    PrefetchPartitionsResponseCallback callback,
    PrefetchPartitionsStatusCallback status_callback) {
  return impl_->PrefetchPartitions(std::move(request), std::move(callback),
                                   std::move(status_callback));
}

client::CancellableFuture<PrefetchPartitionsResponse>
VersionedLayerClient::PrefetchPartitions(
    PrefetchPartitionsRequest request,
    PrefetchPartitionsStatusCallback status_callback) {
  return impl_->PrefetchPartitions(std::move(request),
                                   std::move(status_callback));
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

bool VersionedLayerClient::IsCached(const geo::TileKey& tile,
                                    bool aggregated) const {
  return impl_->IsCached(tile, aggregated);
}

bool VersionedLayerClient::Protect(const TileKeys& tiles) {
  return impl_->Protect(tiles);
}

bool VersionedLayerClient::Release(const TileKeys& tiles) {
  return impl_->Release(tiles);
}

bool VersionedLayerClient::Protect(const std::string& partition_id) {
  return impl_->Protect({partition_id});
}

bool VersionedLayerClient::Release(const std::string& partition_id) {
  return impl_->Release({partition_id});
}

bool VersionedLayerClient::Protect(
    const std::vector<std::string>& partition_ids) {
  return impl_->Protect(partition_ids);
}

bool VersionedLayerClient::Release(
    const std::vector<std::string>& partition_ids) {
  return impl_->Release(partition_ids);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
