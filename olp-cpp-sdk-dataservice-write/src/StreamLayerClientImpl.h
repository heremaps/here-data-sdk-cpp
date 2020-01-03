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
#include <olp/core/client/OlpClientSettings.h>

#include <olp/dataservice/write/StreamLayerClient.h>
#include "generated/model/Catalog.h"

namespace olp {
namespace client {
class CancellationContext;
class PendingRequests;
}  // namespace client

namespace dataservice {
namespace write {

class StreamLayerClientImpl
    : public std::enable_shared_from_this<StreamLayerClientImpl> {
 public:
  StreamLayerClientImpl(client::HRN catalog,
                        StreamLayerClientSettings client_settings,
                        client::OlpClientSettings settings);

  ~StreamLayerClientImpl();

  bool CancelPendingRequests();

  olp::client::CancellableFuture<PublishDataResponse> PublishData(
      model::PublishDataRequest request);
  olp::client::CancellationToken PublishData(model::PublishDataRequest request,
                                             PublishDataCallback callback);

  boost::optional<std::string> Queue(const model::PublishDataRequest& request);
  olp::client::CancellableFuture<StreamLayerClient::FlushResponse> Flush(
      model::FlushRequest request);
  olp::client::CancellationToken Flush(
      model::FlushRequest request, StreamLayerClient::FlushCallback callback);
  size_t QueueSize() const;
  boost::optional<model::PublishDataRequest> PopFromQueue();

  client::CancellableFuture<PublishSdiiResponse> PublishSdii(
      model::PublishSdiiRequest request);

  client::CancellationToken PublishSdii(model::PublishSdiiRequest request,
                                        PublishSdiiCallback callback);

 protected:
  virtual PublishSdiiResponse PublishSdiiTask(
      model::PublishSdiiRequest request, client::CancellationContext context);

  virtual PublishDataResponse PublishDataTask(
      model::PublishDataRequest request, client::CancellationContext context);

  virtual PublishSdiiResponse IngestSdii(model::PublishSdiiRequest request,
                                         client::CancellationContext context);

  // TODO: rename me into PublishDataLessThanTwentyMib once
  // StreamLayerClientImpl will be moved to sync API
  virtual PublishDataResponse PublishDataLessThanTwentyMibSync(
      const model::PublishDataRequest& request,
      client::CancellationContext context);

  virtual PublishDataResponse PublishDataGreaterThanTwentyMibSync(
      const model::PublishDataRequest& request,
      client::CancellationContext context);

  virtual std::string GenerateUuid() const;

 private:
  std::string FindContentTypeForLayerId(const model::Catalog& catalog,
                                        const std::string& layer_id);

  std::string GetUuidListKey() const;

 private:
  client::HRN catalog_;

  client::OlpClientSettings settings_;

  std::shared_ptr<cache::KeyValueCache> cache_;
  mutable std::mutex cache_mutex_;
  StreamLayerClientSettings stream_client_settings_;

  std::shared_ptr<client::PendingRequests> pending_requests_;
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
