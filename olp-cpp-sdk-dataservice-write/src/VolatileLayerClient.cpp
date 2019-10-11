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

#include <olp/dataservice/write/VolatileLayerClient.h>

#include "VolatileLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace write {
VolatileLayerClient::VolatileLayerClient(client::HRN catalog,
                                         client::OlpClientSettings settings)
    : impl_(std::make_shared<VolatileLayerClientImpl>(std::move(catalog),
                                                      std::move(settings))) {}

void VolatileLayerClient::CancellAll() { impl_->CancellAll(); }

olp::client::CancellableFuture<PublishPartitionDataResponse>
VolatileLayerClient::PublishPartitionData(
    model::PublishPartitionDataRequest request) {
  return impl_->PublishPartitionData(request);
}

olp::client::CancellationToken VolatileLayerClient::PublishPartitionData(
    model::PublishPartitionDataRequest request,
    PublishPartitionDataCallback callback) {
  return impl_->PublishPartitionData(request, std::move(callback));
}

olp::client::CancellableFuture<GetBaseVersionResponse>
VolatileLayerClient::GetBaseVersion() {
  return impl_->GetBaseVersion();
}

olp::client::CancellationToken VolatileLayerClient::GetBaseVersion(
    GetBaseVersionCallback callback) {
  return impl_->GetBaseVersion(std::move(callback));
}

olp::client::CancellableFuture<StartBatchResponse>
VolatileLayerClient::StartBatch(model::StartBatchRequest request) {
  return impl_->StartBatch(request);
}

olp::client::CancellationToken VolatileLayerClient::StartBatch(
    model::StartBatchRequest request, StartBatchCallback callback) {
  return impl_->StartBatch(request, std::move(callback));
}

olp::client::CancellableFuture<GetBatchResponse> VolatileLayerClient::GetBatch(
    const model::Publication& pub) {
  return impl_->GetBatch(pub);
}

olp::client::CancellationToken VolatileLayerClient::GetBatch(
    const model::Publication& pub, GetBatchCallback callback) {
  return impl_->GetBatch(pub, std::move(callback));
}

olp::client::CancellableFuture<PublishToBatchResponse>
VolatileLayerClient::PublishToBatch(
    const model::Publication& pub,
    const std::vector<model::PublishPartitionDataRequest>& partitions) {
  return impl_->PublishToBatch(pub, partitions);
}

olp::client::CancellationToken VolatileLayerClient::PublishToBatch(
    const model::Publication& pub,
    const std::vector<model::PublishPartitionDataRequest>& partitions,
    PublishToBatchCallback callback) {
  return impl_->PublishToBatch(pub, partitions, std::move(callback));
}

olp::client::CancellableFuture<CompleteBatchResponse>
VolatileLayerClient::CompleteBatch(const model::Publication& pub) {
  return impl_->CompleteBatch(pub);
}

olp::client::CancellationToken VolatileLayerClient::CompleteBatch(
    const model::Publication& pub, CompleteBatchCallback callback) {
  return impl_->CompleteBatch(pub, std::move(callback));
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
