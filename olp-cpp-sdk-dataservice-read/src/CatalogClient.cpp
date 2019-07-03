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

#include "olp/dataservice/read/CatalogClient.h"
#include <olp/core/cache/DefaultCache.h>
#include "CatalogClientImpl.h"

namespace olp {
namespace dataservice {
namespace read {

std::shared_ptr<cache::KeyValueCache> CreateDefaultCache(
    const cache::CacheSettings& settings) {
  auto cache = std::make_shared<cache::DefaultCache>(settings);
  cache->Open();
  return std::move(cache);
}

CatalogClient::CatalogClient(
    const client::HRN& hrn, std::shared_ptr<client::OlpClientSettings> settings,
    std::shared_ptr<cache::KeyValueCache> cache)
    : impl_(std::make_shared<CatalogClientImpl>(hrn, settings, cache)) {}

bool CatalogClient::cancelPendingRequests() {
  return impl_->CancelPendingRequests();
}

client::CancellationToken CatalogClient::GetCatalog(
    const CatalogRequest& request, const CatalogResponseCallback& callback) {
  return impl_->GetCatalog(request, callback);
}

client::CancellableFuture<CatalogResponse> CatalogClient::GetCatalog(
    const CatalogRequest& request) {
  return impl_->GetCatalog(request);
}

client::CancellationToken CatalogClient::GetCatalogMetadataVersion(
    const CatalogVersionRequest& request,
    const CatalogVersionCallback& callback) {
  return impl_->GetCatalogMetadataVersion(request, callback);
}

client::CancellableFuture<CatalogVersionResponse>
CatalogClient::GetCatalogMetadataVersion(const CatalogVersionRequest& request) {
  return impl_->GetCatalogMetadataVersion(request);
}

client::CancellationToken CatalogClient::GetPartitions(
    const PartitionsRequest& request,
    const PartitionsResponseCallback& callback) {
  return impl_->GetPartitions(request, callback);
}

client::CancellableFuture<PartitionsResponse> CatalogClient::GetPartitions(
    const PartitionsRequest& request) {
  return impl_->GetPartitions(request);
}

client::CancellationToken CatalogClient::GetData(
    const DataRequest& request, const DataResponseCallback& callback) {
  return impl_->GetData(request, callback);
}

client::CancellableFuture<DataResponse> CatalogClient::GetData(
    const DataRequest& request) {
  return impl_->GetData(request);
}

client::CancellationToken CatalogClient::PrefetchTiles(
    const PrefetchTilesRequest& request,
    const PrefetchTilesResponseCallback& callback) {
  return impl_->PrefetchTiles(request, callback);
}

client::CancellableFuture<PrefetchTilesResponse> CatalogClient::PrefetchTiles(
    const PrefetchTilesRequest& request) {
  return impl_->PrefetchTiles(request);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
