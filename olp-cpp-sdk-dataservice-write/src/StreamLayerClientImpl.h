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

#include <olp/dataservice/write/StreamLayerClient.h>
#include "ApiClientLookup.h"
#include "generated/model/Catalog.h"

#include "generated/BlobApi.h"
#include "generated/ConfigApi.h"
#include "generated/IngestApi.h"
#include "generated/PublishApi.h"

namespace olp {
namespace client {
class CancellationContext;
class PendingRequests;
}  // namespace client

namespace dataservice {
namespace write {

using InitApiClientsCallback =
    std::function<void(boost::optional<client::ApiError>)>;
using InitCatalogModelCallback =
    std::function<void(boost::optional<client::ApiError>)>;

class StreamLayerClientImpl
    : public std::enable_shared_from_this<StreamLayerClientImpl> {
 public:
  StreamLayerClientImpl(client::HRN catalog,
                        StreamLayerClientSettings client_settings,
                        client::OlpClientSettings settings);

  ~StreamLayerClientImpl();

  client::CancellableFuture<PublishDataResponse> PublishData(
      const model::PublishDataRequest& request);

  client::CancellationToken PublishData(
      const model::PublishDataRequest& request, PublishDataCallback callback);

  boost::optional<std::string> Queue(const model::PublishDataRequest& request);
  client::CancellableFuture<StreamLayerClient::FlushResponse> Flush(
      model::FlushRequest request);

  client::CancellationToken Flush(model::FlushRequest request,
                                  StreamLayerClient::FlushCallback callback);

  size_t QueueSize() const;

  boost::optional<model::PublishDataRequest> PopFromQueue();

  client::CancellableFuture<PublishSdiiResponse> PublishSdii(
      const model::PublishSdiiRequest& request);

  client::CancellationToken PublishSdii(
      const model::PublishSdiiRequest& request, PublishSdiiCallback callback);

 protected:
  std::string GetUuidListKey() const;

  static std::string FindContentTypeForLayerId(const model::Catalog& catalog,
                                               const std::string& layer_id);

  virtual PublishDataResponse PublishDataTask(
      client::HRN catalog, client::OlpClientSettings settings,
      model::PublishDataRequest request,
      client::CancellationContext cancellation_context);

  virtual PublishSdiiResponse PublishSdiiTask(
      client::HRN catalog, client::OlpClientSettings settings,
      model::PublishSdiiRequest request,
      client::CancellationContext cancellation_context);

  virtual PublishDataResponse PublishDataLessThanTwentyMibTask(
      client::HRN catalog, client::OlpClientSettings settings,
      model::PublishDataRequest request,
      client::CancellationContext cancellation_context);

  virtual PublishDataResponse PublishDataGreaterThanTwentyMibTask(
      client::HRN catalog, client::OlpClientSettings settings,
      model::PublishDataRequest request,
      client::CancellationContext cancellation_context);

  virtual CatalogResponse GetCatalog(client::HRN catalog,
                                     client::CancellationContext context,
                                     model::PublishDataRequest request,
                                     client::OlpClientSettings settings);

  virtual InitPublicationResponse InitPublication(
      client::HRN catalog, client::CancellationContext context,
      model::PublishDataRequest request, client::OlpClientSettings settings);

  virtual SubmitPublicationResponse SubmitPublication(
      client::HRN catalog, client::CancellationContext context,
      model::PublishDataRequest request, std::string publication_id,
      client::OlpClientSettings settings);

  virtual PutBlobResponse PutBlob(client::HRN catalog,
                                  client::CancellationContext context,
                                  model::PublishDataRequest request,
                                  std::string content_type,
                                  std::string data_handle,
                                  client::OlpClientSettings settings);

  virtual UploadPartitionsResponse UploadPartition(
      client::HRN catalog, client::CancellationContext context,
      model::PublishDataRequest request, std::string publication_id,
      model::PublishPartitions partitions, std::string partition_id,
      client::OlpClientSettings settings);

  virtual IngestDataResponse IngestData(client::HRN catalog,
                                        client::CancellationContext context,
                                        model::PublishDataRequest request,
                                        std::string content_type,
                                        client::OlpClientSettings settings);

  virtual PublishSdiiResponse IngestSDII(client::HRN catalog,
                                         client::CancellationContext,
                                         model::PublishSdiiRequest request,
                                         client::OlpClientSettings settings);

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
