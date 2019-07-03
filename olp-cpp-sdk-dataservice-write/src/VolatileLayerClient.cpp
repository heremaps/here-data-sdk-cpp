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
VolatileLayerClient::VolatileLayerClient(
    const client::HRN& catalog, const client::OlpClientSettings& settings)
    : impl_(std::make_shared<VolatileLayerClientImpl>(catalog, settings)) {}

void VolatileLayerClient::cancellAll() { impl_->cancellAll(); }

olp::client::CancellableFuture<PublishPartitionDataResponse>
VolatileLayerClient::PublishPartitionData(
    const model::PublishPartitionDataRequest& request) {
  return impl_->PublishPartitionData(request);
}

olp::client::CancellationToken VolatileLayerClient::PublishPartitionData(
    const model::PublishPartitionDataRequest& request,
    const PublishPartitionDataCallback& callback) {
  return impl_->PublishPartitionData(request, callback);
}

olp::client::CancellableFuture<GetBaseVersionResponse>
VolatileLayerClient::GetBaseVersion() {
  return impl_->GetBaseVersion();
}

olp::client::CancellationToken VolatileLayerClient::GetBaseVersion(
    const GetBaseVersionCallback& callback) {
  return impl_->GetBaseVersion(callback);
}

olp::client::CancellableFuture<StartBatchResponse>
VolatileLayerClient::StartBatch(const model::StartBatchRequest& request) {
  return impl_->StartBatch(request);
}

olp::client::CancellationToken VolatileLayerClient::StartBatch(
    const model::StartBatchRequest& request,
    const StartBatchCallback& callback) {
  return impl_->StartBatch(request, callback);
}

olp::client::CancellableFuture<GetBatchResponse> VolatileLayerClient::GetBatch(
    const model::Publication& pub) {
  return impl_->GetBatch(pub);
}

olp::client::CancellationToken VolatileLayerClient::GetBatch(
    const model::Publication& pub, const GetBatchCallback& callback) {
  return impl_->GetBatch(pub, callback);
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
    const PublishToBatchCallback& callback) {
  return impl_->PublishToBatch(pub, partitions, callback);
}

olp::client::CancellableFuture<CompleteBatchResponse>
VolatileLayerClient::CompleteBatch(const model::Publication& pub) {
  return impl_->CompleteBatch(pub);
}

olp::client::CancellationToken VolatileLayerClient::CompleteBatch(
    const model::Publication& pub, const CompleteBatchCallback& callback) {
  return impl_->CompleteBatch(pub, callback);
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
