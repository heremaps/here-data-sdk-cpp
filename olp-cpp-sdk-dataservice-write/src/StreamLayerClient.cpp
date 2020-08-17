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

#include "olp/dataservice/write/StreamLayerClient.h"

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "StreamLayerClientImpl.h"

namespace olp {
namespace dataservice {
namespace write {

StreamLayerClient::StreamLayerClient(client::HRN catalog,
                                     StreamLayerClientSettings client_settings,
                                     client::OlpClientSettings settings) {
  if (!settings.cache) {
    settings.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }

  impl_ = std::make_shared<StreamLayerClientImpl>(
      std::move(catalog), std::move(client_settings), std::move(settings));
}

void StreamLayerClient::CancelPendingRequests() {
  impl_->CancelPendingRequests();
}

olp::client::CancellableFuture<PublishDataResponse>
StreamLayerClient::PublishData(model::PublishDataRequest request) {
  return impl_->PublishData(request);
}

olp::client::CancellationToken StreamLayerClient::PublishData(
    model::PublishDataRequest request, PublishDataCallback callback) {
  return impl_->PublishData(request, std::move(callback));
}

boost::optional<std::string> StreamLayerClient::Queue(
    model::PublishDataRequest request) {
  return impl_->Queue(request);
}

olp::client::CancellableFuture<StreamLayerClient::FlushResponse>
StreamLayerClient::Flush(model::FlushRequest request) {
  return impl_->Flush(std::move(request));
}

olp::client::CancellationToken StreamLayerClient::Flush(
    model::FlushRequest request, FlushCallback callback) {
  return impl_->Flush(std::move(request), std::move(callback));
}

olp::client::CancellableFuture<PublishSdiiResponse>
StreamLayerClient::PublishSdii(model::PublishSdiiRequest request) {
  return impl_->PublishSdii(request);
}

olp::client::CancellationToken StreamLayerClient::PublishSdii(
    model::PublishSdiiRequest request, PublishSdiiCallback callback) {
  return impl_->PublishSdii(request, std::move(callback));
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
