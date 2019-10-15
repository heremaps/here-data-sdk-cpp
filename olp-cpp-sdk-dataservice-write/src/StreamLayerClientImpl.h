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

#pragma once

#include <mutex>

#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/dataservice/write/FlushSettings.h>
#include <olp/dataservice/write/StreamLayerClient.h>
#include "ApiClientLookup.h"
#include "AutoFlushController.h"
#include "generated/model/Catalog.h"

namespace olp {
namespace client {
class CancellationContext;
}

namespace dataservice {
namespace write {

using InitApiClientsCallback =
    std::function<void(boost::optional<client::ApiError>)>;
using InitCatalogModelCallback =
    std::function<void(boost::optional<client::ApiError>)>;

class StreamLayerClientImpl
    : public std::enable_shared_from_this<StreamLayerClientImpl> {
 public:
  StreamLayerClientImpl(const client::HRN& catalog,
                        const client::OlpClientSettings& settings,
                        const std::shared_ptr<cache::KeyValueCache>& cache);

  olp::client::CancellableFuture<PublishDataResponse> PublishData(
      const model::PublishDataRequest& request);
  olp::client::CancellationToken PublishData(
      const model::PublishDataRequest& request, PublishDataCallback callback);

  boost::optional<std::string> Queue(const model::PublishDataRequest& request);
  olp::client::CancellableFuture<StreamLayerClient::FlushResponse> Flush();
  olp::client::CancellationToken Flush(
      StreamLayerClient::FlushCallback callback);
  size_t QueueSize() const;
  boost::optional<model::PublishDataRequest> PopFromQueue();

  olp::client::CancellableFuture<PublishSdiiResponse> PublishSdii(
      const model::PublishSdiiRequest& request);
  olp::client::CancellationToken PublishSdii(
      const model::PublishSdiiRequest& request, PublishSdiiCallback callback);

 private:
  client::CancellationToken InitApiClients(
      const std::shared_ptr<client::CancellationContext>& cancel_context,
      InitApiClientsCallback callback);
  client::CancellationToken InitApiClientsGreaterThanTwentyMib(
      const std::shared_ptr<client::CancellationContext>& cancel_context,
      InitApiClientsCallback callback);
  client::CancellationToken InitCatalogModel(
      const model::PublishDataRequest& request,
      const InitCatalogModelCallback& callback);
  void InitPublishDataGreaterThanTwentyMib(
      const std::shared_ptr<client::CancellationContext>& cancel_context,
      const model::PublishDataRequest& request,
      const PublishDataCallback& callback);

  void AquireInitLock();
  void NotifyInitAborted();
  void NotifyInitCompleted();

  std::string FindContentTypeForLayerId(const std::string& layer_id);
  client::CancellationToken PublishDataLessThanTwentyMib(
      const model::PublishDataRequest& request,
      const PublishDataCallback& callback);
  client::CancellationToken PublishDataGreaterThanTwentyMib(
      const model::PublishDataRequest& request,
      const PublishDataCallback& callback);
  std::string GetUuidListKey() const;

 private:
  client::HRN catalog_;
  model::Catalog catalog_model_;

  client::OlpClientSettings settings_;

  std::shared_ptr<client::OlpClient> apiclient_config_;
  std::shared_ptr<client::OlpClient> apiclient_ingest_;
  std::shared_ptr<client::OlpClient> apiclient_blob_;
  std::shared_ptr<client::OlpClient> apiclient_publish_;

  std::mutex init_mutex_;
  std::condition_variable init_cv_;
  bool init_inprogress_;

  std::shared_ptr<cache::KeyValueCache> cache_;
  mutable std::mutex cache_mutex_;

  /// These fields are currently not used - will be refactored after 1.0
  FlushSettings flush_settings_;
  std::unique_ptr<AutoFlushController> auto_flush_controller_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
