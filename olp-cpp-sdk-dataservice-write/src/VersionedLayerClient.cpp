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

#include <olp/dataservice/write/VersionedLayerClient.h>

#include "VersionedLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace write {

VersionedLayerClient::VersionedLayerClient(client::HRN catalog,
                                           client::OlpClientSettings settings)
    : impl_(std::make_shared<VersionedLayerClientImpl>(std::move(catalog),
                                                       std::move(settings))) {}


olp::client::CancellableFuture<StartBatchResponse>
VersionedLayerClient::StartBatch(model::StartBatchRequest request) {
  return impl_->StartBatch(request);
}

olp::client::CancellationToken VersionedLayerClient::StartBatch(
    model::StartBatchRequest request, StartBatchCallback callback) {
  return impl_->StartBatch(request, std::move(callback));
}

olp::client::CancellableFuture<GetBaseVersionResponse>
VersionedLayerClient::GetBaseVersion() {
  return impl_->GetBaseVersion();
}

olp::client::CancellationToken VersionedLayerClient::GetBaseVersion(
    GetBaseVersionCallback callback) {
  return impl_->GetBaseVersion(std::move(callback));
}

olp::client::CancellableFuture<GetBatchResponse> VersionedLayerClient::GetBatch(
    const model::Publication& pub) {
  return impl_->GetBatch(pub);
}

olp::client::CancellationToken VersionedLayerClient::GetBatch(
    const model::Publication& pub, GetBatchCallback callback) {
  return impl_->GetBatch(pub, std::move(callback));
}

olp::client::CancellableFuture<CompleteBatchResponse>
VersionedLayerClient::CompleteBatch(const model::Publication& pub) {
  return impl_->CompleteBatch(pub);
}

olp::client::CancellationToken VersionedLayerClient::CompleteBatch(
    const model::Publication& pub, CompleteBatchCallback callback) {
  return impl_->CompleteBatch(pub, std::move(callback));
}

olp::client::CancellableFuture<CancelBatchResponse>
VersionedLayerClient::CancelBatch(const model::Publication& pub) {
  return impl_->CancelBatch(pub);
}

olp::client::CancellationToken VersionedLayerClient::CancelBatch(
    const model::Publication& pub, CancelBatchCallback callback) {
  return impl_->CancelBatch(pub, std::move(callback));
}

void VersionedLayerClient::CancelAll() { impl_->CancelAll(); }

olp::client::CancellableFuture<PublishPartitionDataResponse>
VersionedLayerClient::PublishToBatch(
    const model::Publication& pub, model::PublishPartitionDataRequest request) {
  return impl_->PublishToBatch(pub, request);
}

olp::client::CancellationToken VersionedLayerClient::PublishToBatch(
    const model::Publication& pub, model::PublishPartitionDataRequest request,
    PublishPartitionDataCallback callback) {
  return impl_->PublishToBatch(pub, request, std::move(callback));
}

olp::client::CancellableFuture<CheckDataExistsResponse>
VersionedLayerClient::CheckDataExists(model::CheckDataExistsRequest request) {
  return impl_->CheckDataExists(request);
}

olp::client::CancellationToken VersionedLayerClient::CheckDataExists(
    model::CheckDataExistsRequest request, CheckDataExistsCallback callback) {
  return impl_->CheckDataExists(request, std::move(callback));
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
