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
#include "ApiClientLookup.h"
#include "CancellationTokenList.h"

#include <memory>

#include <olp/dataservice/write/IndexLayerClient.h>
#include "generated/model/Catalog.h"

namespace olp {
namespace dataservice {
namespace write {

namespace model {
class PublishDataRequest;
}

using InitApiClientsCallback =
    std::function<void(boost::optional<client::ApiError>)>;
using InitCatalogModelCallback =
    std::function<void(boost::optional<client::ApiError>)>;

class IndexLayerClientImpl
    : public std::enable_shared_from_this<IndexLayerClientImpl> {
 public:
  IndexLayerClientImpl(const client::HRN& catalog,
                       const client::OlpClientSettings& settings);

  void CancelAll();

  olp::client::CancellableFuture<PublishIndexResponse> PublishIndex(
      const model::PublishIndexRequest& request);

  olp::client::CancellationToken PublishIndex(
      model::PublishIndexRequest request, const PublishIndexCallback& callback);

  olp::client::CancellationToken DeleteIndexData(
      const model::DeleteIndexDataRequest& request,
      const DeleteIndexDataCallback& callback);

  olp::client::CancellableFuture<DeleteIndexDataResponse> DeleteIndexData(
      const model::DeleteIndexDataRequest& request);

  olp::client::CancellableFuture<UpdateIndexResponse> UpdateIndex(
      const model::UpdateIndexRequest& request);

  olp::client::CancellationToken UpdateIndex(
      const model::UpdateIndexRequest& request,
      const UpdateIndexCallback& callback);

 private:
  client::CancellationToken InitApiClients(
      std::shared_ptr<client::CancellationContext> cancel_context,
      InitApiClientsCallback callback);

  client::CancellationToken InitCatalogModel(
      const model::PublishIndexRequest& request,
      const InitCatalogModelCallback& callback);

  std::string FindContentTypeForLayerId(const std::string& layer_id);

 private:
  client::HRN catalog_;
  model::Catalog catalog_model_;

  client::OlpClientSettings settings_;

  std::shared_ptr<client::OlpClient> apiclient_config_;
  std::shared_ptr<client::OlpClient> apiclient_blob_;
  std::shared_ptr<client::OlpClient> apiclient_index_;

  CancellationTokenList tokenList_;

  std::mutex mutex_;
  std::condition_variable cond_var_;

  bool init_in_progress_;
};
}  // namespace write
}  // namespace dataservice
}  // namespace olp
