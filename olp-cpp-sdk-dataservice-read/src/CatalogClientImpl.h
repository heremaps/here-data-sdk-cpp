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

#include <future>
#include <memory>

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>

namespace olp {
namespace client {
class OlpClient;
struct OlpClientSettings;
class PendingRequests;
}  // namespace client

namespace dataservice {
namespace read {

class PrefetchTilesProvider;
namespace repository {
class CatalogRepository;
class PartitionsRepository;
class DataRepository;
}  // namespace repository

class CatalogClientImpl final {
 public:
  CatalogClientImpl(client::HRN catalog, client::OlpClientSettings settings);

  ~CatalogClientImpl();

  bool CancelPendingRequests();

  client::CancellationToken GetCatalog(const CatalogRequest& request,
                                       const CatalogResponseCallback& callback);

  client::CancellableFuture<CatalogResponse> GetCatalog(
      const CatalogRequest& request);

  client::CancellationToken GetCatalogMetadataVersion(
      const CatalogVersionRequest& request,
      const CatalogVersionCallback& callback);

  client::CancellableFuture<CatalogVersionResponse> GetCatalogMetadataVersion(
      const CatalogVersionRequest& request);

  client::CancellationToken GetPartitions(
      const PartitionsRequest& request,
      const PartitionsResponseCallback& callback);

  client::CancellableFuture<PartitionsResponse> GetPartitions(
      const PartitionsRequest& request);

  client::CancellationToken GetData(const DataRequest& request,
                                    const DataResponseCallback& callback);

  client::CancellableFuture<DataResponse> GetData(const DataRequest& request);

  client::CancellationToken PrefetchTiles(
      const PrefetchTilesRequest& request,
      const PrefetchTilesResponseCallback& callback);

  client::CancellableFuture<PrefetchTilesResponse> PrefetchTiles(
      const PrefetchTilesRequest& request);

 private:
  client::HRN catalog_;
  std::shared_ptr<client::OlpClientSettings> settings_;
  std::shared_ptr<repository::CatalogRepository> catalog_repo_;
  std::shared_ptr<repository::PartitionsRepository> partition_repo_;
  std::shared_ptr<repository::DataRepository> data_repo_;
  std::shared_ptr<PrefetchTilesProvider> prefetch_provider_;
  std::shared_ptr<client::PendingRequests> pending_requests_;

  template <typename Request, typename Response>
  client::CancellableFuture<Response> AsFuture(
      const Request& request, std::function<client::CancellationToken(
                                  CatalogClientImpl*, const Request&,
                                  const std::function<void(Response)>&)>
                                  func) {
    auto promise = std::make_shared<std::promise<Response> >();

    auto cancel_token = func(this, request, [promise](Response response) {
      promise->set_value(response);
    });

    return client::CancellableFuture<Response>(cancel_token, promise);
  }
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
