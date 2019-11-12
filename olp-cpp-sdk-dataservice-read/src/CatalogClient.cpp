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

#include "CatalogClientImpl.h"
#include "olp/core/cache/DefaultCache.h"
#include "olp/core/client/OlpClientSettingsFactory.h"

namespace olp {
namespace dataservice {
namespace read {

CatalogClient::CatalogClient(client::HRN catalog,
                             client::OlpClientSettings settings) {
  // If the user did not specify cache, we creade a default one (memory only)
  if (!settings.cache) {
    settings.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }

  impl_ = std::make_shared<CatalogClientImpl>(std::move(catalog),
                                              std::move(settings));
}

CatalogClient::CatalogClient(CatalogClient&& other) noexcept = default;

CatalogClient& CatalogClient::operator=(CatalogClient&& other) noexcept =
    default;

bool CatalogClient::CancelPendingRequests() {
  return impl_->CancelPendingRequests();
}

client::CancellationToken CatalogClient::GetCatalog(
    CatalogRequest request, CatalogResponseCallback callback) {
  return impl_->GetCatalog(std::move(request), std::move(callback));
}

client::CancellableFuture<CatalogResponse> CatalogClient::GetCatalog(
    CatalogRequest request) {
  return impl_->GetCatalog(std::move(request));
}

client::CancellationToken CatalogClient::GetLatestVersion(
    CatalogVersionRequest request, CatalogVersionCallback callback) {
  return impl_->GetLatestVersion(std::move(request), std::move(callback));
}

client::CancellableFuture<CatalogVersionResponse>
CatalogClient::GetLatestVersion(CatalogVersionRequest request) {
  return impl_->GetLatestVersion(std::move(request));
}

client::CancellationToken CatalogClient::GetCatalogMetadataVersion(
    CatalogVersionRequest request, CatalogVersionCallback callback) {
  return impl_->GetLatestVersion(std::move(request), std::move(callback));
}

client::CancellableFuture<CatalogVersionResponse>
CatalogClient::GetCatalogMetadataVersion(CatalogVersionRequest request) {
  return impl_->GetLatestVersion(std::move(request));
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
}  // namespace read
}  // namespace dataservice
}  // namespace olp
