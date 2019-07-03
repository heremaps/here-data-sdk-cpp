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

#include <olp/dataservice/write/IndexLayerClient.h>

#include "IndexLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace write {
IndexLayerClient::IndexLayerClient(const client::HRN& catalog,
                                   const client::OlpClientSettings& settings)
    : impl_(std::make_shared<IndexLayerClientImpl>(catalog, settings)) {}

void IndexLayerClient::CancelAll() { impl_->CancelAll(); }

olp::client::CancellableFuture<PublishIndexResponse>
IndexLayerClient::PublishIndex(const model::PublishIndexRequest& request) {
  return impl_->PublishIndex(request);
}

olp::client::CancellationToken IndexLayerClient::PublishIndex(
    const model::PublishIndexRequest& request,
    const PublishIndexCallback& callback) {
  return impl_->PublishIndex(request, callback);
}

olp::client::CancellationToken IndexLayerClient::DeleteIndexData(
    const model::DeleteIndexDataRequest& request,
    const DeleteIndexDataCallback& callback) {
  return impl_->DeleteIndexData(request, callback);
}

olp::client::CancellableFuture<DeleteIndexDataResponse>
IndexLayerClient::DeleteIndexData(
    const model::DeleteIndexDataRequest& request) {
  return impl_->DeleteIndexData(request);
}

olp::client::CancellableFuture<UpdateIndexResponse>
IndexLayerClient::UpdateIndex(const model::UpdateIndexRequest& request) {
  return impl_->UpdateIndex(request);
}

olp::client::CancellationToken IndexLayerClient::UpdateIndex(
    const model::UpdateIndexRequest& request,
    const UpdateIndexCallback& callback) {
  return impl_->UpdateIndex(request, callback);
}
}  // namespace write
}  // namespace dataservice
}  // namespace olp
