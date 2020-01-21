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

#include "olp/dataservice/read/StreamLayerClient.h"

#include <olp/core/porting/make_unique.h>
#include "StreamLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace read {

StreamLayerClient::StreamLayerClient(client::HRN catalog, std::string layer_id,
                                     client::OlpClientSettings settings)
    : impl_(std::make_unique<StreamLayerClientImpl>(
          std::move(catalog), std::move(layer_id), std::move(settings))) {}

StreamLayerClient::StreamLayerClient(StreamLayerClient&& other) noexcept =
    default;

StreamLayerClient& StreamLayerClient::operator=(
    StreamLayerClient&& other) noexcept = default;

StreamLayerClient::~StreamLayerClient() = default;

bool StreamLayerClient::CancelPendingRequests() {
  return impl_->CancelPendingRequests();
}

client::CancellationToken StreamLayerClient::Subscribe(
    SubscribeRequest request, SubscribeResponseCallback callback) {
  return impl_->Subscribe(std::move(request), std::move(callback));
}

client::CancellableFuture<SubscribeResponse> StreamLayerClient::Subscribe(
    SubscribeRequest request) {
  return impl_->Subscribe(std::move(request));
}

client::CancellationToken StreamLayerClient::Unsubscribe(
    UnsubscribeResponseCallback callback) {
  return impl_->Unsubscribe(std::move(callback));
}

olp::client::CancellableFuture<UnsubscribeResponse>
StreamLayerClient::Unsubscribe() {
  return impl_->Unsubscribe();
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
